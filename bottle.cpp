/**
 * Bottle class.
 */

#include <Arduino.h>

#include "bottle.h"
#include "ads1231.h"
#include "utils.h"
#include "errors.h"
#include "config.h"


/*
 * Init bottles.
 * Attach servos and turn to initial position.
 * 'bottles' is an array (size 'bottles_nr') of Bottle objects.
 */
void Bottle::init(Bottle* bottles, int bottles_nr) {
    for (int i=0; i < bottles_nr; i++) {
        bottles[i].servo.attach(bottles[i].pin);
        bottles[i].servo.writeMicroseconds(bottles[i].pos_up); // Make sure bottle is pointing up
        delay(500);
    }
}


/**
 * Bottle constructor.
 * Initialize values of member variables. Note: Servo cannot be used here.
 * This should be done in the setup function, not in the global scope using
 * bottles_init().
 */
Bottle::Bottle(unsigned char _number, unsigned char _pin, int _pos_down, int _pos_up) :
    number(_number), pin(_pin), pos_down(_pos_down), pos_up(_pos_up) {
}

/**
 * Turn servo towards 'pos' in 1 microsecond steps, waiting delay_ms
 * milliseconds between steps (speed = 1/delay). If check_weight weight
 * is true, might abort with WHERE_THE_FUCK_IS_THE_CUP error. If a valid pointer
 * stable_weight is passed, turns bottle until a stable weight is measured
 * (returns WEIGHT_NOT_STABLE if pos is reached before weight stable).
 *
 * Returns 0 when the position is reached or SERVO_OUT_OF_RANGE on error.
 *
 * For details about the built-in Servo class see:
 *     /usr/share/arduino/libraries/Servo/Servo.cpp
 *
 */
errv_t Bottle::turn_to(int pos, int delay_ms, bool check_weight, int* stable_weight, bool enable_abortcheck) {
    int weight_previous1 = -9999;  // just any impossible value
    int weight_previous2 = -9999;  // ..before we have real values

    if (pos < SERVO_MIN || pos > SERVO_MAX) {
        DEBUG_MSG_LN("Invalid pos");
        return SERVO_OUT_OF_RANGE;
    }

    int current_pos = servo.readMicroseconds();
    if (pos == current_pos)
        return 0;
    int step = (current_pos < pos) ? 1 : -1;

    DEBUG_START();
    DEBUG_MSG("turn ");
    DEBUG_MSG(number);
    DEBUG_MSG(", params ");
    DEBUG_VAL(current_pos);
    DEBUG_VAL(step);
    DEBUG_VAL(pos);
    DEBUG_VAL(delay_ms);
    DEBUG_END();
    unsigned long last_called = millis();
    for (int i = current_pos + step; i * step <= pos * step; i += step) {
        //                             ˆˆˆˆˆˆ        ˆˆˆˆˆˆ
        //                             this inverts the relation if turning down

        // Warning: printing to serial delays turning!
        // Might help to to debug servo movement. Not necessary now, commenting
        // out to save bytes.
        //if (print_steps && i % 10 == 0) {
        //    DEBUG_VAL_LN(i);
        //}

        // check abort only if not already aborted...
        if (enable_abortcheck) {
            // turn up and return if we should abort...
            errv_t ret = check_aborted();
            if (ret) {
                // turn_up might not be necessary here, called another time
                // later (does not matter if called twice)
                turn_up(FAST_TURN_UP_DELAY, false);
                return ret;
            }
        }

        #ifndef WITHOUT_SCALE
        if (check_weight || stable_weight) {
            int weight;
            int ret = ads1231_get_noblock(weight);
            if (ret == 0) {
                // we got a valid weight from scale
                if (check_weight && weight < WEIGHT_EPSILON) {
                    return WHERE_THE_FUCK_IS_THE_CUP;
                }

                // get next weight sample and return if weight is stable
                if (stable_weight) {
                    if (weight_previous2 == weight_previous1
                            && weight_previous1 == weight) {
                        *stable_weight = weight;
                        return 0;
                    }
                    weight_previous2 = weight_previous1;
                    weight_previous1 = weight;
                }
            }
            else if (ret != ADS1231_WOULD_BLOCK) {
                // ignoring if it would take too long to get weight, but
                // return in case of other error != 0
                return ret;
            }
        }
        #endif

        // turn servo one step
        delay(delay_ms);
        servo.writeMicroseconds(i);
    }

    // pos reached before weight stable
    if (stable_weight) {
        return WEIGHT_NOT_STABLE;
    }

    return 0;
}

/**
 * Turn bottle to upright position.
 */
errv_t Bottle::turn_up(int delay_ms, bool enable_abortcheck) {
    return turn_to(pos_up, delay_ms, false, NULL, enable_abortcheck);
}

/**
 * Turn bottle to pouring position.
 */
errv_t Bottle::turn_down(int delay_ms, bool check_weight) {
    return turn_to(pos_down, delay_ms, check_weight);
}

/**
 * Returns the pause position for this bottle (unit: microseconds)
 */
int Bottle::get_pause_pos()
{
	return (pos_down + pos_up) / 2;
}

/**
 * Turn bottle to pause position.
 * Used e.g. in case of WHERE_THE_FUCK_IS_THE_CUP error.
 */
errv_t Bottle::turn_to_pause_pos(int delay_ms) {
    return turn_to(get_pause_pos(), delay_ms);
}


/**
 * Pour requested_amount grams from bottle..
 * Return 0 on success, other values are return values of
 * delay_until (including scale error codes).
 */
errv_t Bottle::pour(int requested_amount, int& measured_amount) {
    // orig_weight is weight including ingredients poured until now
    int orig_weight = 0;
    int ret = 0;
    #ifndef WITHOUT_SCALE
    while (1) {
        // get weight while turning bottle, because ads1231_stable_millis()
        // blocks bottle in pause position too long
        int below_pause = (pos_down + get_pause_pos()) / 2;
        ret = turn_to(below_pause, TURN_DOWN_DELAY, true, &orig_weight);

        // Note that checking weight here is a critical issue, allows hacking
        // the robot. If a heavy weight is placed while measuring and then
        // removed while pouring (results in more alcohol). Stable weight
        // should resolve most problems.
        if (ret == WEIGHT_NOT_STABLE) {
            ret = ads1231_get_stable_grams(orig_weight);
        }
        if (ret != 0 && ret != WHERE_THE_FUCK_IS_THE_CUP) {
            return ret;
        }
        if (ret == WHERE_THE_FUCK_IS_THE_CUP || orig_weight < WEIGHT_EPSILON) {
            // no cup...
            RETURN_IFN_0(wait_for_cup());
        }
        else {
            // everything fine
            break;
        }
    }
    #endif

    // loop until successfully poured or aborted or other fatal error
    while(1) {
        // petres wants POURING message also after resume...
        // https://github.com/rfjakob/barwin-arduino/issues/10
        MSG(String("POURING ") + String(number) + String(" ") + String(orig_weight));

        DEBUG_MSG_LN("Turn down");
        ret = turn_down(TURN_DOWN_DELAY, true); // enable check_weight

        // wait for requested weight
        // FIXME here we do not want WEIGHT_EPSILON and sharp >
        if (ret == 0) {
            DEBUG_MSG_LN("Waiting");
            #ifndef WITHOUT_SCALE
                ret = delay_until(POURING_TIMEOUT,
                        orig_weight + requested_amount - UPGRIGHT_OFFSET, true);
            #else
                ret = delay_abortable(requested_amount * MS_PER_GRAMS);
            #endif
        }
        if (ret == 0)
            break; // All good

        DEBUG_MSG_LN(String("pour: got err ") + String(ret));

        // Bottle empty
        // Note that this does not work if requested_amount is less than
        // UPGRIGHT_OFFSET!
        if(ret == BOTTLE_EMPTY) {
            ERROR(c_strerror(BOTTLE_EMPTY) + String(" ") + String(number) );
            // TODO other speed here? it is empty already!
            RETURN_IFN_0(turn_to(pos_up + BOTTLE_EMPTY_POS_OFFSET, TURN_UP_DELAY));
            RETURN_IFN_0(wait_for_resume()); // might return ABORTED
	    print_lcd("", 2);  // clear error (writing POUR command again would 
	                       // be nicer, but difficult...)
        }
        // Cup was removed early
        else if(ret == WHERE_THE_FUCK_IS_THE_CUP) {
            ERROR(c_strerror(WHERE_THE_FUCK_IS_THE_CUP));
            RETURN_IFN_0(turn_to_pause_pos(FAST_TURN_UP_DELAY));
            RETURN_IFN_0(wait_for_cup());
	    print_lcd("", 2);  // clear error (writing POUR command again would 
	                       // be nicer, but difficult...)
        }
        // other error - turn bottle up and return error code
        // includes: scale error, user abort, ...
        else {
            return ret;
        }
    }

    // We turn to pause pos and not completely up so we can crossfade
    RETURN_IFN_0(turn_to_pause_pos(TURN_UP_DELAY));

    #ifndef WITHOUT_SCALE
    RETURN_IFN_0(ads1231_get_grams(measured_amount));
    measured_amount -= orig_weight;
    #else
    // this is not a real measurement, but best we can do not break the
    // protocol and keep everything backward compatible without scale...
    measured_amount = requested_amount;
    #endif

    DEBUG_START();
    DEBUG_MSG("Stats: ");
    DEBUG_VAL(requested_amount);
    DEBUG_VAL(measured_amount);
    DEBUG_END();

    return 0;
}

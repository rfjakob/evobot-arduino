/**
 * Bottle class.
 */

#ifndef BOTTLE_H
#define BOTTLE_H

// 'pos' should be always between following two constants, when calling
// Bottle::turn_to(pos, delay_ms). Edit this values according to used servos.
// The constants might be used to initialize pos_down and pos_up (in config.h).
// See also:
// http://arduino.cc/en/Reference/ServoWriteMicroseconds
#define SERVO_MIN 1000
#define SERVO_MAX 2000

#include <Servo.h>

// Macro initialising a array 'bottles' of Bottle instances as defined
// in config.h by the comma separated list BOTTLES of constructor calls:
#define DEFINE_BOTTLES()  Bottle bottles[] = {BOTTLES};\
                          int bottles_nr = sizeof(bottles)/sizeof(bottles[0]);



class Bottle {
    public:
        Bottle(String, int, int, int);
        static void init(Bottle* bottles, int bottles_nr);
        int turn_to(int pos, int delay_ms, bool print_steps=false);
        int turn_up(int delay_ms, bool print_steps=false);
        int turn_down(int delay_ms, bool print_steps=false);
        Servo servo;          // servo used for turning the bottle
        const String name;    // human readable name, e.g "Vodka"
        const char pin;       // pin to attach the servo
    private:
        const int pos_down;   // servo position for bottle down (pouring)
        const int pos_up;     // servo position for bottle up (not pouring)
};

#endif

#ifndef UTILS_H
#define UTILS_H

#define DEBUG
#ifdef DEBUG
    /**
     * This a bunch of macros as ugly work-a-round because there is no printf.
     * Usage:
     *      - Use DEBUG_START()  and DEBUG_END() in the beginning/end of a
     *        debug line and in between only DEBUG_MSG() and DEBUG_VAL().
     *      - Use DEBUG_MSG(msg) to print strings or variables' content.
     *      - Use DEBUG_VAL(val) to print a variables name and its content.
     *      - For a single variable/string you can simply use the commands
     *        DEBUG_MSG_LN(msg) adn DEBUG_VAL_LN(val) without DEBUG_START
     *        and DEBUG_END().
     */
    #define DEBUG_START() Serial.print("DEBUG ")
    #define DEBUG_END() Serial.println()

    #define DEBUG_MSG(msg) Serial.print(msg)
    #define DEBUG_VAL(val) do { Serial.print(#val); \
                                Serial.print(": "); \
                                Serial.print(val);  \
                                Serial.print(", "); \
                              } while (0)

    #define DEBUG_MSG_LN(msg) do { DEBUG_START(); \
                                   Serial.println(msg); \
                                 } while (0)
    #define DEBUG_VAL_LN(val) do { DEBUG_START(); \
                                   DEBUG_VAL(val); \
                                   DEBUG_END(); \
                                 } while (0)
#else
    // disable all debug output on serial interface...
    #define DEBUG_START()
    #define DEBUG_MSG(msg)
    #define DEBUG_MSG_LN(msg)

    #define DEBUG_VAL(val)
    #define DEBUG_VAL_LN(val)
#endif


// add space at the end and make things shorter
// TODO add space at the end?
#define MSG(msg) Serial.println(msg)

#define ERROR(msg) do { Serial.print("ERROR "); \
                        Serial.print(msg); \
                        Serial.println(" "); \
                      } while (0)

#endif

// Call something and return error code if !=0
// e.g.: RETURN_IFN_0(delay_until(...));
#define RETURN_IFN_0(code) do { \
                                int ret = code; \
                                if (ret != 0)   \
                                    return ret; \
                              } while(0)


bool has_time_passed(long time_period, long& last_passed);

/**
 * A quite dangerous macro, to simplify usage of has_time_passed().
 *
 * Example usage:
 * loop() {
 *      IF_HAS_TIME_PASSED(500) {
 *          do_something_every_500ms();
 *      }
 * }
 */
#define IF_HAS_TIME_PASSED(time_period) \
        static long HAS_TIME_PASSED_last_passed = millis();\
        if (has_time_passed(time_period, HAS_TIME_PASSED_last_passed))


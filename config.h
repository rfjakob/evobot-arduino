/*
 * Configuration file for the evobot Arduino code
 */

#ifndef CONFIG_H
#define CONFIG_H


#define ADS1231_CLK_PIN  1
#define ADS1231_DATA_PIN 2
// ADC counts per milligram
#define ADS1231_DIVISOR  1
// Zero offset, milligrams
#define ADS1231_OFFSET   0

// PWM pin for servo number 1
#define SERVO1_PIN      13

#endif

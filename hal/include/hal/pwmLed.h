#ifndef _PWM_LED_H_
#define _PWM_LED_H_

#include "stdbool.h"

int initPwmLed();
void setLed(const int period, const int duty_cycle);
void updatePwmLedState(const bool state);

#endif
#include "hal/pwmLed.h"
#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"

#define PWM0B_DUTY_CYCLE "/dev/bone/pwm/0/b/duty_cycle"
#define PWM0B_DUTY_ENABLE "/dev/bone/pwm/0/b/enable"
#define PWM0B_PERIOD "/dev/bone/pwm/0/b/period"


void setLed(const int period, const int duty_cycle){

    FILE *fDutyCycle = fopen(PWM0B_DUTY_CYCLE, "w");
    FILE *fPeriod = fopen(PWM0B_PERIOD, "w");

    int fDutyCycleWritten;
    int fPeriodWritten;
    if (!fDutyCycle || !fPeriod){
        exit(-1);
    }
    fDutyCycleWritten = fprintf(fDutyCycle, "%d", duty_cycle);
    fPeriodWritten = fprintf(fPeriod, "%d", period);

     if (fDutyCycleWritten <= 0) {
        exit(1);
    }
     if (fPeriodWritten <= 0) {
        exit(1);
    }
    // Close file
    fclose(fDutyCycle);
    fclose(fPeriod);

}
int initPwmLed(){

    FILE *fDutyCycle = fopen(PWM0B_DUTY_CYCLE, "w");
    FILE *fPeriod = fopen(PWM0B_PERIOD, "w");
    FILE *fEnable = fopen(PWM0B_DUTY_ENABLE, "w");

    if (!fEnable || !fDutyCycle || !fPeriod){
        return -1;
    }
    int fDutyCycleWritten = fprintf(fDutyCycle, "0");
    int fPeriodWritten = fprintf(fPeriod, "0");
    int fEnableWritten = fprintf(fEnable, "1");


    if (fEnableWritten <= 0 || fDutyCycleWritten <= 0|| fPeriodWritten <= 0) {
        return -1;
    }
    // Close file
    fclose(fEnable);
    return 0;
}

void updatePwmLedState(const bool state){ //call in CS
    FILE *fPin = fopen(PWM0B_DUTY_ENABLE, "w");
    int fPinWritten;
    if (!fPin){
        exit(1);
    }
    fPinWritten = fprintf(fPin, "%d", state);


    if (fPinWritten <= 0) {
        exit(1);
    }
    // Close file
    fclose(fPin);
}
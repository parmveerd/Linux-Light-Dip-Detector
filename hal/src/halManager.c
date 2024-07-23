#include "hal/halManager.h"
#include "hal/pwmLed.h"
#include "hal/segDisplay.h"
#include <stdio.h>
#include <stdlib.h>


static const char* command = "config-pin P9_18 i2c & config-pin P9_17 i2c & config-pin p9_22 pwm & config-pin p9_21 pwm";

// Initialize P9_18 & P9_17 using I2C for segDisplay
// Initialize P9_21 and P9_22 using PWM for pwmLed and potDriver
//static void initPins(void);

static int initPins(){
       // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1000]; 
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        //printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
        return -1;
    }
    return 0;
}
int initHal(int* flag){
    if(initPins() < 0){
        perror("Error initializing pins for I2C and PWM");
        return -1;
    }; // initialize all pins for I2C and PWM
    if(initPwmLed() < 0){//initialize PwM LED
        perror("Error initializing LED using PWM");
        return -1;
    } 
    if(initSegDisplay(flag) < 0) { // initialize display state and setup GPIO
         perror("Error initializing SegDisplay module");
         return -1;
    } 
    return 0;
}
int shutdownHal(){
    if(cleanupSegDisplay() < 0){
         perror("Error cleaning up SegDisplay");
         return -1;
    }
    updatePwmLedState(0); //disable pwm so it stops flashing
    return 0;
}

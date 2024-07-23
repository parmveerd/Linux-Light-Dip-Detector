#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "hal/sensorDriver.h"
#define A2D_FILE_VOLTAGE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"


int getA2DSensorReading(){

    // Open file
    FILE *f = fopen(A2D_FILE_VOLTAGE1, "r");
    if (!f){
        printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    // Get reading
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0)
    {
        printf("ERROR: Unable to read values from voltage input file.\n");
        exit(-1);
    }
    // Close file
    fclose(f);
    return a2dReading;
}
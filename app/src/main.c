// Main program to build the application

#include <stdio.h>
#include "hal/potDriver.h"
#include "hal/pwmLed.h"
#include "hal/sensorDriver.h"
#include "time.h"
#include "hal/halManager.h"
#include "stdbool.h"
#include <stdlib.h>
#include "udpServer.h"
#include <pthread.h>
#include <sampler.h>
#include "hal/segDisplay.h"


static int flag = 1; // used to keep track of all busy-wait thread loops


int main()
{

    if(initHal(&flag) < 0){
        perror("Problem occured while initializing Hardware Abstraction Layer, exiting...\n");
        exit(1);
    }
    if(initUdpServer(&flag) < 0){
        perror("Problem occured while initializing UDP Server, exiting...\n");
        exit(1);
    }
    if(Sampler_init(&flag) < 0){
        perror("Problem occured while initializing Sampler, exiting...\n");
        exit(1);
    }
    

    shutdownUdpServer();
    Sampler_cleanup();
    shutdownHal();


    return 0;

}
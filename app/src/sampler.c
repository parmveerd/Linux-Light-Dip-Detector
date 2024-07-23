#include "sampler.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "hal/sensorDriver.h"
#include "hal/potDriver.h"
#include "periodTimer.h"
#include "hal/pwmLed.h"
#include "hal/segDisplay.h"

static pthread_t tSamplingPID;
static pthread_t updateValues;
static pthread_t displayValues;
static pthread_t ledFlicker;

static pthread_mutex_t samplerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;

static Period_statistics_t statistics;

static int *continueFlag;

static double currValue = 0;
static double currValues[2000];
static double historyValues[2000];
static int secSize = 0;
static int hz = 0;
static int potValue = 0;
static double prevAvg = 0;
static double prevVoltage = 0;
static double currAvg = 0;
static double currVoltage = 0;
static double secDips = 0;
static double sum = 0;
static int dips = 0;
static int length = 0;
static double minPeriod = 0;
static double maxPeriod = 0;
static double avgPeriod = 0;
static long long count = 0;

static void sampler_join(void);


typedef struct {
    double *values; // array of samples
} HistoryData;

HistoryData historyData;
static long long historyDataSize = 0;
static long long historyDataCapacity = 1000;
static long long totalCount = 0;


static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

// returns the number of dips in the last second
int getDips() {
    return dips;
}
static void* ledFlash();
static void* updateData();
static void* displayData();

//Use this to find the current reading of light every 1ms
void* sampleData(){
    while (*continueFlag) {
        pthread_mutex_lock(&samplerMutex);
        {
            // start the period time
            Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
            // get the light sample, find its volts, place in array
            currValue = getA2DSensorReading();
            currValue = currValue / 4095 * 1.8;
            currValues[secSize] = currValue;
            secSize++;
            sleepForMs(1);
        }
        pthread_mutex_unlock(&samplerMutex);
    }
    
    return NULL;
}

// Use this to update all of our current and prev values
static void* updateData() {
    while (*continueFlag) {
        // update the previous values
        prevAvg = currAvg;
        prevVoltage = currVoltage;

        pthread_mutex_lock(&samplerMutex);
        {
            // update current values
            currVoltage = currValue;
            sum += currValue;
            count += 1;

            // update current average
            if (count > 1) {
                currAvg = (sum/count)*0.001 + prevAvg*(1-0.001);
            } else {
                currAvg = sum/count;
            }

            // update dips if there is one
            if ((prevAvg - prevVoltage <= 0.03) && (currAvg - currVoltage >= 0.1) && (secSize > 1)) {
                secDips += 1;
            }
        }
        pthread_mutex_unlock(&samplerMutex);

    }
   
    return NULL;
}

// Will sleep for 1 sec then print all of our values
static void* displayData() {
    while (*continueFlag) {
        // sleep for 1 second
        sleepForMs(1000);

        // update our dips and samples per sec
        dips = secDips;
        length = secSize;
        
        // reset old values back to 0 
        currVoltage = 0;
        prevVoltage = 0;
        secSize = 0;
        secDips = 0;

        // call period to clear
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &statistics);

        pthread_mutex_lock(&printMutex);
        {   
            // move curr to history
            for (int i = 0; i < length; i++) {
                historyValues[i] = currValues[i];
            }

            // find the pot values and turn to frequency
            potValue = getA2DPotReading();
            hz = potValue/40;
            // update seg display with new dips
            segDisplaySetDip(dips);

            // find the avg, max, and min periods
            avgPeriod = statistics.avgPeriodInMs;
            maxPeriod = statistics.maxPeriodInMs;
            minPeriod = statistics.minPeriodInMs;

            // print out the samples to the terminal
            printf("#Smpl/s = %d\tPOT @ %d=> %dHz\tavg = %.3fV\tdips = %d\tSmpl ms[%.3f, %.3f] avg %.3f/%d\n", length, potValue, hz, currAvg, dips, minPeriod, maxPeriod, avgPeriod, length);

            // print out 20 evenly spaced values to the terminal
            int items = 0;
            int temp = length;
            int add = temp/20;

            if (temp <= 20) {
                add = 1;
            }

            for (int i = 0; i < temp; i += add) {
                if (items < 20) {
                    printf("\t%d:%.3f\t", i , historyValues[i]);
                }
                items++;
            }
            printf("\n");

            // move our temp history values into our history struct for storage
            for (int i = 0; i < length; i++) {
                historyData.values[i] = historyValues[i];
            }
            historyDataSize = length+1;
            totalCount += length;
        }
        pthread_mutex_unlock(&printMutex);
    }
    
    return NULL;
}

// Flashes LEDs at desired frequency, contolled by POT
static void* ledFlash() {
    int prevReading = getA2DPotReading();
    double frequency = 1.0;
    int reading;
    int period;
    int duty_cycle;
    while (*continueFlag) {
        // get our current pot value
        reading = potValue;
        // if different than prev, then update freq, period, and duty cycle
        if(prevReading != reading){
            prevReading = reading;

            if(reading < 40){
                frequency = 1.0;
            }
            else{
                frequency = (reading / 40.0); // Compute desired frequency from potentiometer reading
            }
            period = (1.0 / frequency) * 1000000000.0;
            duty_cycle = period / 2;
            setLed(period, duty_cycle);
        }
        sleepForMs(100); // sleep for 0.1s at this frequency, so it does not change too fast
    }
    
    return NULL;
}

// Begin/end the background thread which samples light levels.
int Sampler_init(int *flag){
    // initialize our struct values size
    historyData.values = (double *)malloc(historyDataCapacity * sizeof(double));

    // update our while loop flag
    continueFlag = flag;

    Period_init();

    pthread_mutex_init(&samplerMutex, NULL);
    pthread_mutex_init(&printMutex, NULL);

    // create all of our threads so they can run
    if (pthread_create(&tSamplingPID, NULL, &sampleData, NULL) != 0){
        perror("Error creating tSamplingPID thread\n");
        return -1;
    }
    if (pthread_create(&updateValues, NULL, &updateData, NULL) != 0){
        perror("Error creating updateValues thread\n");
        return -1;

    }
    if (pthread_create(&displayValues, NULL, &displayData, NULL) != 0){
        perror("Error creating displayValues thread\n");
        return -1;

    }
    if (pthread_create(&ledFlicker, NULL, &ledFlash, NULL) != 0){
        perror("Error creating ledFlicker thread\n");
        return -1;

    }

    return 1;
}

// join function
static void sampler_join(void) {
    pthread_join(tSamplingPID, NULL);
    pthread_join(updateValues, NULL);
    pthread_join(displayValues, NULL);
    pthread_join(ledFlicker, NULL);
}

void Sampler_cleanup(void){
    sampler_join();
    free(historyData.values); // free dynamic memory
    historyData.values = NULL;
    Period_cleanup();

    // destroy all locks
    pthread_mutex_destroy(&samplerMutex);
    pthread_mutex_destroy(&printMutex);
}

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void Sampler_moveCurrentDataToHistory(void){
    // already do this in the displayData() function
    // can just move that from there to here
    // but make sure to run this function every 1s or will lose samples

}
// Get the number of samples collected during the previous complete second.
int Sampler_getHistorySize(void){
    return length;
}
// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* Sampler_getHistory(int *size){
    double* copy = NULL;

    pthread_mutex_lock(&printMutex);

    copy = (double*)malloc(historyDataSize * sizeof(double));
    if (copy == NULL) {
        *size = 0;
    } else {
        for (int i = 0; i < historyDataSize; ++i) {
            copy[i] = historyData.values[i];
        }
        *size = historyDataSize;
    }

    pthread_mutex_unlock(&printMutex);
    return copy;    
}

// Get the average light level (not tied to the history).
double Sampler_getAverageReading(void){
    return prevAvg;
}
// Get the total number of light level samples
long long Sampler_getNumSamplesTaken(void) {
    return totalCount;
}

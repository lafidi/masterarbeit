//
// Created by lars on 10.05.2019.
//

#ifndef EKG_SERVER_GLOBAL_H
#define EKG_SERVER_GLOBAL_H

#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "kissfft/kiss_fftr.h"

#include <websock/websock.h>

#include <pigpio.h>

typedef struct {
    int ecgI;
    int ecgII;
    int ecgIII;
    int ecgAVR;
    int ecgAVL;
    int ecgAVF;
    int ecgResp;
} dataECG;

typedef struct {
    float Hb;
    float HbO2;
} dataPPG;

extern float temp_C;

typedef struct {
    int ecgFD;
    int ppgFD;
    int tempFD;
    libwebsock_context *ctx;
} dataFD;

//declaration of global functions
dataFD setup (int ecgChannel, int ppgChannel, int tempChannel, char* port);
int swapBytes(int word);

//declaration of PPG functions
int ppgSetup (int deviceID);
int ppgRead (int ppgFD);
dataPPG ppgMeanCalculation (dataPPG array[], int size);
dataPPG ppgRMSCalculation (dataPPG array[], int size);
dataPPG ppgFindMax (dataPPG array[], int size);
dataPPG ppgFindMin (dataPPG array[], int size);
dataPPG ppgPeakDetection (dataPPG array[], int size, dataPPG lastValue, int threshold, int distance);

//declaration of ECG functions
int ecgSetup (int channel);
dataECG ecgRead (int ecgFD);
int ecgPeakDetection (dataECG array[], int size, dataECG lastValue, int threshold, int distance);
float ecgRMSCalculation (dataECG array[], int size);
int respirationPeakDetection (int array[], int size);

//declaration of temperature functions
int temperatureSetup (int channel);
void* temperateRead (void* arg);

//declaration of websocket functions
libwebsock_context *websocketStart (char *port);
void* websocketWait (void* ctx);
void websocketSever (libwebsock_context *ctx, dataECG* ecgData, dataPPG* ppgData, float dataTemp, int numberIterations,
                     float spo2Value, int ecgFrequency, int ppgFrequency, int respirationFrequency);

#endif //EKG_SERVER_GLOBAL_H

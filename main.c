//
// Created by lars on 10.05.2019.
//

#include "global.h"

float temp_C;

//chance MUX to IR
static inline void enableIR(void){
    //gpioWrite(20, 1);
    gpioWrite(21, 1);
    //gpioWrite(20, 0);
}

//chance MUX to red
static inline void enableRed(void) {
    //gpioWrite(20, 1);
    gpioWrite(21, 0);
    //gpioWrite(20, 0);
}

//turn off MUX
static inline void disableLED(void) {
    gpioWrite(20, 1);
}

//turn on MUX
static inline void enableLED(void) {
    gpioWrite(20, 0);
}

/***
 * @brief absolute main function, needed for everything
 *
 * @return 1 on success, negative else
 */
int main(void){

    //call main setup function
    dataFD fdData = setup (0x00, 0x48, 0x01, (char *) "55555");

    //check if all connections to devices could be established
    if ((fdData.tempFD < 0) | (fdData.ecgFD < 0) | (fdData.ppgFD < 0)){
        exit(-1);
    }

    //set needed variables
    //TODO clear variables
    int iterations = 30;
    int outerIterations = 7;
    dataECG ecgData[iterations];
    dataPPG ppgData[iterations];
    dataPPG ppgFrequencyCounterInner;
    int ecgFrequencyCounterInner;
    dataPPG ppgFrequencyLastValue;
    dataECG ecgFrequencyLastValue;
    int ppgFrequencyCounterOuter;
    int ecgFrequencyCounterOuter;
    int ppgFrequency;
    int ecgFrequency;
    int respirationFrequency;
    dataPPG ppgDataStore[outerIterations*iterations];
    int respirationDataStore[outerIterations*iterations];
    //dataECG ecgDataStore[outerIterations*iterations];
    float spo2Value = 0;
    dataPPG ppgMean;
    //dataPPG ppgRMS;
    clock_t start, end;
    double cpu_time_used;

    //if websocket thread is running, tell so
    pthread_t threadIDWebSocket;
    pthread_create (&threadIDWebSocket, NULL, websocketWait, (void *) fdData.ctx);
    printf ("running WebSocket...\r\n");

    //if temperature reading thread is running, tell so
    pthread_t threadIDWebSocketTemperature;
    pthread_create (&threadIDWebSocketTemperature, NULL, temperateRead, (void *) &(fdData.tempFD));
    printf ("running temperature...\r\n");

    printf("Server ready to serve\r\n");

    ppgFrequency = 0;
    respirationFrequency = 0;
    ecgFrequency = 0;
    //float ambient;

    while (true) {
        ppgFrequencyCounterOuter = 0;
        ecgFrequencyCounterOuter = 0;
        start = time(0);
        for (int i = 0; i < outerIterations; i++) {
            for (int j = 0; j < iterations; j++) {
                //ambient = ((float) ppgRead (fdData.ppgFD));
                enableLED ();
                enableRed ();
                usleep (1);
                ppgData[j].Hb = ((float) ppgRead (fdData.ppgFD));
                enableIR ();
                usleep (1);
                ppgData[j].HbO2= ((float) ppgRead (fdData.ppgFD));
                disableLED();
                //ppgData[j].HbO2 -= ambient;
                //ppgData[j].Hb -= ambient;


                /*ecgDataStore[i * iterations + j] =*/ ecgData[j] = ecgRead (fdData.ecgFD);
                respirationDataStore[i * iterations + j] = ecgData[j].ecgResp;

                usleep (1000000 * (1.0 / (30.0 * iterations)));

                //printf("Hb: %f, HbO2: %f\r\n", ppgMean.Hb, ppgMean.HbO2);
            }

            ppgFrequencyCounterInner = ppgPeakDetection (ppgData, iterations, ppgFrequencyLastValue, 10, 5);
            ecgFrequencyCounterInner = ecgPeakDetection (ecgData, iterations, ecgFrequencyLastValue, 1500, 5);
            ppgFrequencyLastValue = ppgData[iterations-1];
            ecgFrequencyLastValue = ecgData[iterations-1];
            /*if (ppgFrequencyCounterInner.HbO2 > ppgFrequencyCounterInner.Hb){
                ppgFrequencyCounterOuter += ppgFrequencyCounterInner.Hb;
            } else {
                ppgFrequencyCounterOuter += ppgFrequencyCounterInner.HbO2;
            }*/
            ppgFrequencyCounterOuter += ppgFrequencyCounterInner.HbO2;
            ecgFrequencyCounterOuter += ecgFrequencyCounterInner;

            for (int j = 0; j < iterations; j++) {
                ppgDataStore[i * iterations + j].HbO2 = ppgData[j].HbO2 = ppgData[j].HbO2 * 3.3 / 65536.0 / 20000.0;
                ppgDataStore[i * iterations + j].Hb = ppgData[j].Hb = ppgData[j].Hb * 3.3 / 65536.0 / 20000.0;
            }

            //TODO: This is not really working
            //ppgRMS = ppgRMSCalculation (ppgData, iterations);
            //spo2Value = log (ppgData[5].Hb) / log (ppgData[5].HbO2);
            //printf("SpO2: %f       ", spo2Value);
            //printf("SpO2: %f\r\n", spo2Value);
            //spo2Value = (ppgRMS.HbO2/ppgMean.HbO2)/(ppgRMS.Hb/ppgMean.Hb);
            //printf("SpO2: %f       ", spo2Value);
            //spo2Value = (ppgRMS.HbO2/ppgData[i].HbO2)/(ppgRMS.Hb/ppgData[i].Hb);
            //printf("SpO2: %f       ", spo2Value);
            //printf("SpO2: %f\r\n", ((-45.06*spo2Value+30.354)*spo2Value+94.845));

            //printf("Hb: %.10f, HbO2: %.10f\r\n", ppgRMS.Hb, ppgRMS.HbO2);


            websocketSever (fdData.ctx, ecgData, ppgData, temp_C, iterations, spo2Value, ecgFrequency, ppgFrequency, respirationFrequency);
        }

        end = time(0);

        cpu_time_used = end - start;

        ppgFrequency = ((float) ppgFrequencyCounterOuter * (1.0 / cpu_time_used * 60.0));
        ecgFrequency = ((float) ecgFrequencyCounterOuter * (1.0 / cpu_time_used * 60.0));
        respirationFrequency = ((float) respirationPeakDetection (respirationDataStore, outerIterations*iterations) * (1.0 / cpu_time_used * 60.0));

        ppgMean = ppgMeanCalculation (ppgDataStore, outerIterations*iterations);
        dataPPG ppgMin = ppgFindMin (ppgDataStore, outerIterations*iterations);
        dataPPG ppgMax = ppgFindMax (ppgDataStore, outerIterations*iterations);
        spo2Value = ((ppgMax.HbO2 - ppgMin.HbO2) / (ppgMean.HbO2)) / ((ppgMax.Hb - ppgMin.Hb) / (ppgMean.Hb));
        //float ratio = (log(ppgMin.HbO2/ppgMax.HbO2)) / (log(ppgMin.Hb/ppgMax.Hb));
        printf("SpO2: %f\r\n", spo2Value);

        /*for (int j = 0; j < outerIterations*iterations; j++) {
            ppgDataStore[j].HbO2 = (ppgDataStore[j].HbO2 - ppgMean.HbO2);
            ppgDataStore[j].Hb = (ppgDataStore[j].Hb - ppgMean.Hb);
        }*/

        /*ppgRMS = ppgRMSCalculation (ppgDataStore, outerIterations);
        dataPPG ppgMin = ppgFindMin (ppgDataStore, outerIterations);
        dataPPG ppgMax = ppgFindMax (ppgDataStore, outerIterations);
        float ratio = ((ppgMax.HbO2 - ppgMin.HbO2) / ppgRMS.HbO2) / ((ppgMax.Hb - ppgMin.Hb) / ppgRMS.Hb);
        spo2Value = (int) (95.842 - (1/ratio * 22.6));
        spo2Value = log (ppgRMS.HbO2) / log (ppgRMS.Hb);
        printf("SpO2: %f\r\n", spo2Value);*/

    }

    return 1;

}
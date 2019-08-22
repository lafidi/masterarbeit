//
// Created by lars on 10.05.2019.
//

#include "global.h"

/**
 * @brief initialize ECG
 *
 * This function initializes the ECG device with given settings so that conversions can take place whenever needed.
 *
 * @param channel (int) channel of the device on SPI bus
 * @return channel or errorCode
 */
int ecgSetup (int channel) {
    int ecgFD;
    int errorHandler; /**< takes return value of currently called function to check for errors */
    char buf[4]; /**< takes return value from SPI operations as well as sending value */

    //open SPI connection
    int flags = (1 << 1) + 1; /**< SPI mode 3 of device 0 */
    ecgFD = spiOpen (channel, 2000000, flags);
    if (ecgFD < 0)
        return ecgFD;

    //Hardware rese
    //cannot be used as this GPIO is needed for LEDs toggling of PPG part
    /*gpioSetMode (21, PI_OUTPUT);
    usleep (15 * 1000);
    usleep (100 * 1000);
    gpioWrite (21, 1);
    usleep (100 * 1000);
    gpioWrite (21, 0);
    usleep (100 * 1000);
    gpioWrite (21, 1);
    usleep (100 * 1000);*/

    //Software reset
    buf[0] = 0x81;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x01;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    //set ~DRDY
    gpioSetMode (20, PI_INPUT);
    gpioSetPullUpDown (20, PI_PUD_OFF);

    //setting according to manual
    //CMREFCTL is set to 0x85E0000B
    buf[0] = 0x85;
    buf[1] = 0xE0;
    buf[2] = 0x00;
    buf[3] = 0x0B;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    //FRMCTL is set to 0x8A1F9600
    buf[0] = 0x8A;
    buf[1] = 0x1F;
    buf[2] = 0x86; //CE = pace disabled, C6 = pace disabled LOFF enabled
    buf[3] = 0x00;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    //ECGCTL is set to 0x81E004AE
    buf[0] = 0x81;
    buf[1] = 0xE0;
    buf[2] = 0x04;
    buf[3] = 0xAE;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
/*
    buf[0] = buf[1] = buf[2] = buf[3] = 0;
    char buf2[4];
    buf2[0] = 0x01; buf2[1] = buf2[2] = buf2[3] = 0x00;
    spiXfer(ecgFD, buf2, buf, 4);
    printf("SPI: %02x %02x %02x %02x\r\n", buf[0], buf[1], buf[2], buf[3]);
    buf[0] = buf[1] = buf[2] = buf[3] = 0;
    buf2[0] = buf2[1] = buf2[2] = buf2[3] = 0x00;
    spiXfer(ecgFD, buf2, buf, 4);
    printf("SPI: %02x %02x %02x %02x\r\n", buf[0], buf[1], buf[2], buf[3]);
*/

    //RESPCTL is set to 0x83002099
    buf[0] = 0x83;
    buf[1] = 0x00;
    buf[2] = 0x06;
    buf[3] = 0x01;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    //LOFFCTL is set to 0x82000015
    buf[0] = 0x82;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x15;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    /*
    //Testtone
    //CMREFCTL is set to 0x8500000B
    buf[0] = 0x85;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x0B;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
    //TESTTONE is set to 0x88E0000D
    buf[0] = 0x88;
    buf[1] = 0xE0;
    buf[2] = 0x00;
    buf[3] = 0x15;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
    //FILTCTL is set to 0x8B000008
    buf[0] = 0x8B;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x08;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
    //FRMCTL is set to 0x8A1F9610
    buf[0] = 0x8A;
    buf[1] = 0x1F;
    buf[2] = 0x96;
    buf[3] = 0x10;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
    //ECGCTL is set to 0x81E000AE
    buf[0] = 0x81;
    buf[1] = 0xE0;
    buf[2] = 0x00;
    buf[3] = 0xAE;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }
    */

    //FRAMES is set to 0x40000000
    buf[0] = 0x40;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    errorHandler = spiWrite (ecgFD, buf, 4);
    if (errorHandler < 0) {
        return errorHandler;
    }

    return ecgFD;
}

/**
 * @brief initiate ECG data readout
 *
 * This function is used to initiate the ECG data readout when called. Also Goldberger leads are calculated.
 * As the current IC supports it, respirationMagnitude is also recorded and saved for frequency calculation.
 *
 * @param ecgFD `(int)` filedescriptor for SPI connection
 * @return ECG data
 */
dataECG ecgRead (int ecgFD) {
    dataECG data;
    data.ecgResp = data.ecgAVR = data.ecgAVL = data.ecgAVF = data.ecgI = data.ecgII = data.ecgIII = 0;
    bool lead1, lead2, lead3; /**< to record if lead was taken from curent transmission */

    lead1 = lead2 = lead3 = false; /**< set all to false because none of them was received in this round */

    char buf[11][4] = {0}; /**< transmitting buffer for receiving */
    char buf2[5][4] = {0}; /**< transmitting buffer for sending*/

    buf2[0][0] = 0x11; /**< read lead I */
    buf2[1][0] = 0x12; /**< read lead II */
    buf2[2][0] = 0x13; /**< read lead III */
    buf2[3][0] = 0x1B; /**< read respiration */
    buf2[4][0] = 0x40; /**< read stream data */

    /**
     * read all needed registers
     */
    spiXfer (ecgFD, buf2[4], buf[0], 4);
    spiXfer (ecgFD, buf2[4], buf[1], 4);
    spiXfer (ecgFD, buf2[4], buf[2], 4);
    spiXfer (ecgFD, buf2[4], buf[3], 4);
    spiXfer (ecgFD, buf2[4], buf[4], 4);
    spiXfer (ecgFD, buf2[4], buf[5], 4);
    spiXfer (ecgFD, buf2[4], buf[6], 4);
    spiXfer (ecgFD, buf2[4], buf[7], 4);
    spiXfer (ecgFD, buf2[4], buf[8], 4);
    spiXfer (ecgFD, buf2[4], buf[9], 4);
    spiXfer (ecgFD, buf2[4], buf[10], 4);

    for (int j = 0; j < 11; j++) { /**< iterate over all read values */

        if (buf[j][0] == 0x11) { /**< save lead I*/
            //Lead I
            data.ecgI = (int) ((buf[j][1] << 16) + (buf[j][2] << 8) + (buf[j][3] << 0));
            lead1 = true;
        } else if (buf[j][0] == 0x12) { /**< save lead II*/
            //Lead II
            data.ecgII = (int) ((buf[j][1] << 16) + (buf[j][2] << 8) + (buf[j][3] << 0));
            lead2 = true;
        } else if (buf[j][0] == 0x13) { /**< save lead III*/
            //Lead III
            data.ecgIII = (int) ((buf[j][1] << 16) + (buf[j][2] << 8) + (buf[j][3] << 0));
            lead3 = true;
        } else if (buf[j][0] == 0x1B) { /**< save respiration*/
            //Resp mag.
            data.ecgResp = (int) ((buf[j][1] << 16) + (buf[j][2] << 8) + (buf[j][3] << 0));
        } else if (buf[j][0] == 0x1D) { /**< Lead-off frame, curently ignored*/
            //LOFF
            int mask = 0xF0;
            int tmp = (buf[j][1] & mask) >> 4;
            if (tmp == 0x00) {
                //Leads connected
                //printf ("No data\r\n");
            } else {
                //Leads disconnected
                /*printf ("Leads disconnected:");
                if (tmp & 0x8) {
                    printf (" RLD");
                }
                if (tmp & 0x4) {
                    printf (" LA");
                }
                if (tmp & 0x2) {
                    printf (" LL");
                }
                if (tmp & 0x1) {
                    printf (" RA");
                }
                printf ("\r\n");*/
            }
        }
    }

    if ((lead1) && (lead2) &&
        (lead3)) { /**< iff all leads are present, Goldberger leads can be calculated and values saved for frequency detection */
        data.ecgAVR = -(data.ecgI + data.ecgII) / 2; /**< calculate aVR */
        data.ecgAVL = (data.ecgI - data.ecgIII) / 2; /**< calculate aVL */
        data.ecgAVF = (data.ecgII + data.ecgIII) / 2; /**< calculate aVF */

    }

    return data;
}

/**
 * @brief calculate maximum value of lead III
 *
 * @param array `(dataECG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return maximum of lead III
 */
int ecgFindMax (dataECG array[], int size) {
    int maximum;
    maximum = array[0].ecgIII; /**< take first value as (current) maximum */

    for (int c = 0; c < size; c++) { /**< iterate over all values to find global maximum in given cutout */
        if (array[c].ecgIII > maximum) { /**< if current value is larger than max value set it as new max and save it's position */
            maximum = array[c].ecgIII; /**< if current value is larger than max value set it as new max and save it's position */
        }
    }

    return maximum; /**< return position of maximum value */
}

/**
 * @brief calculate minimum value of lead III
 *
 * @param array `(dataECG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return minimum of lead III
 */
int ecgFindMin (dataECG array[], int size) {
    int minimum;
    minimum = array[0].ecgIII; /**< take first value as (current) minimum */

    for (int c = 0; c < size; c++) { /**< iterate over all values to find global maximum in given cutout */
        if (array[c].ecgIII < minimum) { /**< if current value is smaller than min value set it as new min and save it's position */
            minimum = array[c].ecgIII; /**< if current value is smaller than min value set it as new min and save it's position */
        }
    }

    return minimum; /**< return position of maximum value */
}

/**
 * @brief finds number of peaks in lead III
 *
 * @param array `(dataECG[])` all current data to analyze
 * @param size `(int)` length of data
 * @param lastValue `(dataECG)` last value of previous analyze call
 * @param threshold `(int)` vertical step between two points to be considered as peak
 * @param distance `(int)` minimum horizontal distance between two peaks
 * @return number of peaks
 */
int ecgPeakDetection (dataECG array[], int size, dataECG lastValue, int threshold, int distance){
    int result;
    int i = 1;
    result = 0;

    //find peaks using RMS --> no thresholding
    float RMS = ecgRMSCalculation(array, size);
    RMS = (RMS - ecgFindMin(array, size)) * 2.5 + ecgFindMin(array, size);
    i = 0;
    for (; i<size; i++){
        if (array[i].ecgIII > (RMS)) {
            result++;
            i += distance;
        }
    }

    //find peaks using simple thresholding algorithm
    /*if ((array[0].ecgII - lastValue.ecgII) > threshold){
        //if ((array[1].ecgI - array[0].ecgI) > threshold) {
            result++;
            i += distance;
        //}
    }
    for (; i<size; i++){
        if ((array[i].ecgII - array[i-1].ecgII) > threshold){
            //if ((array[i+1].ecgI - array[i].ecgI) > threshold) {
                result++;
                i += distance;
            //}
        }
    }*/

    return result;
}

/**
 * @brief calculates RMS of lead III
 *
 * @param array `(dataECG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return RMS value as float
 */
float ecgRMSCalculation (dataECG array[], int size) {
    long long sum = 0; /**< temporary variable for summing */

    //summing up values in array
    for (int i = 0; i < size; i++) {
        sum += ((long long) array[i].ecgIII * array[i].ecgIII);
    }

    //sqrt(sqsum/numberOfElements) is RMS
    return (sqrt(sum/((float) size)));
}

/**
 * @brief calculates RMS of respiration
 *
 * @param array `(int[])` all current data to analyze
 * @param size `(int)` length of data
 * @return RMS value as float
 */
float respirationRMSCalculation (int array[], int size) {
    long long sum = 0; /**< temporary variable for summing */

    //summing up values in array
    for (int i = 0; i < size; i++) {
        sum += ((long long) array[i] * array[i]);
    }

    //sqrt(sqsum/numberOfElements) is RMS
    return (sqrt(sum/((float) size)));
}

/**
 * @brief calculates mean of respiration
 *
 * @param array `(int[])` all current data to analyze
 * @param size `(int)` length of data
 * @return mean value as float
 */
float respirationMeanCalculation (int array[], int size) {
    long long sum = 0; /**< temporary variable for summing */

    //summing up values in array
    for (int i = 0; i < size; i++) {
        sum += ((long long) array[i]);
    }

    //sqrt(sqsum/numberOfElements) is RMS
    return (sum/((float) size));
}

/**
 * @brief finds number of peaks in respiration
 *
 * @param array `(int[])` all current data to analyze
 * @param size `(int)` length of data
 * @return number of peaks
 */
int respirationPeakDetection (int array[], int size){
    int result = 0;
    int side = 0;

    float rms = respirationRMSCalculation(array, size);

    result = 0;
    if (array[0] > rms){
        side = 1;
    } else {
        side = -1;
    }
    for (int i = 0; i<size; i++){
        if (array[i] > rms){
            if (side < 0) {
                result++;
            }
            side = 1;
        } else {
            side = -1;
        }
    }

    return result;
}



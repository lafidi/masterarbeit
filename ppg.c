//
// Created by lars on 10.05.2019.
//

//using pigpio (http://abyz.me.uk/rpi/pigpio/cif.html) for interaction with sensors and more convenient usage
//#include <pigpio.h>

#include "global.h"

/**
 * @brief initialize PPG
 *
 * This function initializes the PPG device with given settings so that conversions can take place whenever needed.
 *
 * @param deviceID (int) address of the device on I2C bus
 * @return deviceHandler or errorCode
 */
int ppgSetup (int deviceID) {

    int ppgFD; /**< keeps device handler for return and internal purposes */

    if ((ppgFD = i2cOpen(1, deviceID, 0)) < 0){
        return -1;
    }

    //Software-Reset
    i2cWriteWordData (ppgFD, 0x01, swapBytes (0xC5E3));

    //Pin 21 for toggling between IR and red
    gpioSetMode(21, PI_OUTPUT);
    gpioSetPullUpDown(21, PI_PUD_OFF);
    //Pin 20 for turning on/off
    gpioSetMode(20, PI_OUTPUT);
    gpioSetPullUpDown(20, PI_PUD_OFF);

    return ppgFD;
}

/**
 * @brief PPG data readout when called
 *
 * This function is used to read out the PPG data whenever called.
 *
 * @param ppgFD `(int)` descriptor of I2C connection
 * @return current ADC data
 */
int ppgRead (int ppgFD) {
    int data;

    //start conversion
    //bit 15    Start conversation
    //bit 14:12 MUX: AIN_P=AIN0; AIN_N=GND
    //bit 11:9  PGA: standard-value for internal amplifier
    //bit 8     MODE: single shot
    //bit 7:5   DR: highest sampling rate
    //bit 4     COMP_MODE: standard
    //bit 3     COMP_POL: standard
    //bit 2     COMP_LAT: standard
    //bit 1:0   COMP_QUE: standard
    i2cWriteWordData (ppgFD, 0x01, swapBytes (0xC5E3));

    //wait short time for conversion
    usleep(1.0/200.0 * 1000000);

    //Wait for data ready which is indicated on bit 15 going low
    do {;} while (!(swapBytes (i2cReadWordData(ppgFD, 0x01)) & 0x8000));

    //save digital value
    data = swapBytes (i2cReadWordData (ppgFD, 0x00));

    //return digital value
    return (data);
}

/**
 * @brief finds number of peaks
 *
 * @param array `(dataPPG[])` all current data to analyze
 * @param size `(int)` length of data
 * @param lastValue `(dataPPG)` last value of previous analyze call
 * @param threshold `(int)` vertical step between two points to be considered as peak
 * @param distance `(int)` minimum horizontal distance between two peaks
 * @return number of peaks with given parameters for both curves
 */
dataPPG ppgPeakDetection (dataPPG *array, int size, dataPPG lastValue, int threshold, int distance){
    dataPPG result;
    int i = 1;

    //Simple thresholding algorithm
    result.HbO2 = result.Hb = 0;
    if ((array[0].HbO2 - lastValue.HbO2) > threshold){
        //if ((array[1].HbO2 - array[0].HbO2) > threshold) {
            result.HbO2++;
            i += distance;
        //}
    }
    if ((array[0].Hb - lastValue.Hb) > threshold){
        //if ((array[1].Hb - array[0].Hb) > threshold) {
            result.Hb++;
            i += distance;
        //}
    }
    for (; i<size; i++){
        if ((array[i].HbO2 - array[i-1].HbO2) > threshold){
            //if ((array[i+1].HbO2 - array[i].HbO2) > threshold) {
                result.HbO2++;
                i += distance;
            //}
        }
        if ((array[i].Hb - array[i-1].Hb) > threshold){
            //if ((array[i+1].Hb - array[i].Hb) > threshold) {
                result.Hb++;
                i += distance;
            //}
        }
    }

    return result;
}

/**
 * @brief calculate maximum value of PPG
 *
 * @param array `(dataPPG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return maximum of PPG
 */
dataPPG ppgFindMax (dataPPG array[], int size) {
    dataPPG maximum;
    maximum.HbO2 = array[0].HbO2; /**< take first value as (current) maximum */
    maximum.Hb = array[0].Hb; /**< take first value as (current) maximum */

    for (int c = 0; c < size; c++) { /**< iterate over all values to find global maximum in given cutout */
        if (array[c].HbO2 > maximum.HbO2) { /**< if current value is larger than max value set it as new max and save it's position */
            maximum.HbO2 = array[c].HbO2; /**< if current value is larger than max value set it as new max and save it's position */
        }
        if (array[c].Hb > maximum.Hb) { /**< if current value is larger than max value set it as new max and save it's position */
            maximum.Hb = array[c].Hb; /**< if current value is larger than max value set it as new max and save it's position */
        }
    }

    return maximum; /**< return position of maximum value */
}

/**
 * @brief calculate minimum value of PPG
 *
 * @param array `(dataPPG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return minimum of PPG
 */
dataPPG ppgFindMin (dataPPG array[], int size) {
    dataPPG minimum;
    minimum.HbO2 = array[0].HbO2; /**< take first value as (current) maximum */
    minimum.Hb = array[0].Hb; /**< take first value as (current) maximum */

    for (int c = 0; c < size; c++) { /**< iterate over all values to find global maximum in given cutout */
        if (array[c].HbO2 < minimum.HbO2) { /**< if current value is larger than max value set it as new max and save it's position */
            minimum.HbO2 = array[c].HbO2; /**< if current value is larger than max value set it as new max and save it's position */
        }
        if (array[c].Hb < minimum.Hb) { /**< if current value is larger than max value set it as new max and save it's position */
            minimum.Hb = array[c].Hb; /**< if current value is larger than max value set it as new max and save it's position */
        }
    }

    return minimum; /**< return position of maximum value */
}

/**
 * @brief calculates RMS of PPG
 *
 * @param array `(dataPPG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return RMS value as float
 */
dataPPG ppgRMSCalculation (dataPPG array[], int size) {
    float sumHb = 0; /**< temporary variable for summing */
    float sumHbO2 = 0; /**< temporary variable for summing */

    //summing up values in array
    for (int i = 0; i < size; i++) {
        sumHb += ((float) array[i].Hb * array[i].Hb);
        sumHbO2 += ((float) array[i].HbO2 * array[i].HbO2);
    }

    //sqrt(sqsum/numberOfElements) is RMS
    dataPPG result;
    result.HbO2 = sqrt(sumHbO2/((float) size));
    result.Hb = sqrt(sumHb/((float) size));
    return result;
}

/**
 * @brief calculates mean of PPG
 *
 * @param array `(dataPPG[])` all current data to analyze
 * @param size `(int)` length of data
 * @return mean value as float
 */
dataPPG ppgMeanCalculation (dataPPG array[], int size) {
    float sumHb = 0; /**< temporary variable for summing */
    float sumHbO2 = 0; /**< temporary variable for summing */

    //summing up values in array
    for (int i = 0; i < size; i++) {
        sumHb += array[i].Hb;
        sumHbO2 += array[i].HbO2;
    }

    //sum/numberOfElements is mean
    dataPPG result;
    result.HbO2 = sumHbO2/size;
    result.Hb = sumHb/size;
    return result;
}

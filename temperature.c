//
// Created by lars on 10.05.2019.
//

#include "global.h"

/**
 * @brief initialize temperature sensor
 *
 * This function initializes the temperature device with given settings so that conversions can take place whenever needed.
 *
 * @param channel (int) channel of the device on SPI bus
 * @return channel or errorCode
 */
int temperatureSetup (int channel) {
    int tempFD;
    int errorHandler; /**< takes return value of currently called function to check for errors */
    char buffer[4];  /**< takes sending value of SPI operations */

    int flags = (1 << 1) + 1; /**< SPI mode 3 of device 0 */
    tempFD = spiOpen (channel, 2000000, flags);
    if (tempFD < 0)
        return tempFD;

    //set ~DRDY
    gpioSetMode (18, PI_INPUT);
    gpioSetPullUpDown (18, PI_PUD_OFF);

    //turn on device
    buffer[0] = (0x80 | 0x00);
    buffer[1] = 0x01;
    errorHandler = spiWrite (tempFD, buffer, 2);
    if (errorHandler < 0) {
        return errorHandler;
    }

    //config as T type thermocouple
    buffer[0] = (0x80 | 0x01);
    buffer[1] = 0x07;
    errorHandler = spiWrite (tempFD, buffer, 2);
    if (errorHandler < 0) {
        return errorHandler;
    }

    usleep (0.1 * 1000000); /**< sleep to get things applied on sensor */

    return tempFD;
}

/**
 * @brief initiate temperature data readout using pThreads
 *
 * This function is used to initiate the temperature data readout regularly using pThreads and an infinite loop. There are 1/10 readouts per second.
 * If keepRunning == false, infinite loop is stopped and pthread_exit(NULL) called to destroy own thread.
 * To readout the temperature certain registers have to be set first.
 *
 * @param arg `(void*)` standard pthread parameter, here: converted to `*(int*)` channel
 * @return NULL
 */
void* temperateRead (void* arg) {
    int tempFD = *((int *) arg);  /**< have channel in an easy-to-use form */

    int errorHandler; /**< takes return value of currently called function to check for errors */
    char buffer[4]; /**< takes sending value for SPI operations */
    char buff2[4]; /**< takes return value for SPI operations */

    int temp; /**< variable for "raw" temperature */
    int iterationLimiterOuter = 0;

    while (true) {

        do { /**< tries to catch errors if temperature is to high or too low*/
            iterationLimiterOuter++;
            int iterationLimiterInner = 0;

            do { /**< tries to catch errors if registers are different to what they were set */
                iterationLimiterInner++;

                //single readout
                buffer[0] = 0x80;
                buffer[1] = 0x43;
                errorHandler = spiWrite (tempFD, buffer, 2);
                if (errorHandler < 0) {
                    return NULL;
                }

                usleep (400 * 1000); /**< wait 400 ms so temperature conversion is definitively finished */

                //check if registers are right so that you can say the value might be true
                buffer[0] = 0x00;
                buffer[1] = 0x00;
                buffer[2] = 0x00;
                buffer[3] = 0x00;
                errorHandler = spiXfer (tempFD, buffer, buff2, 4);
                if (errorHandler < 0) {
                    return NULL;
                }

            } while ((((buff2[1] != 0x01) & (buff2[1] != 0x41)) | (buff2[2] != 0x07)) & (iterationLimiterInner < 5));

            //read temperature from sensor
            buffer[0] = 0x0C;
            buffer[1] = 0x0D;
            buffer[2] = 0x0E;
            buffer[3] = 0x00;
            errorHandler = spiXfer (tempFD, buffer, buff2, 4);
            if (errorHandler < 0) {
                return NULL;
            }

            //convert bytes to number
            temp = (buff2[1] << 16);
            temp |= buff2[2] << 8;
            temp |= buff2[3];
            temp = temp >> 5;

            temp_C = (float) (temp * 0.0078125); /**< convert number to temperature with factor from manual */


        } while (((temp_C > 100) | (temp_C < 10)) & (iterationLimiterOuter < 5));

        if (iterationLimiterOuter == 5) {
            temp_C = 0;
        }

        usleep (1000000);

    }

    return arg;
}

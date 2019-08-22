//
// Created by lars on 10.05.2019.
//

#include "global.h"

/**
 * @brief general setup routine
 *
 * This is the general setup route. It sets up GPIO usage and defines all pins to be used with the Broadcom GPIO numbers.
 *
 * @param ecgChannel `(int)` SPI channel number for ECG
 * @param ppgChannel `(int)` I2C channel number for ADC
 * @param tempChannel `(int)` SPI channel number for temperature
 * @param port `(char*)` port number for websocket
 * @return file descriptor struct of the devices
 */
dataFD setup (int ecgChannel, int ppgChannel, int tempChannel, char* port) {
    dataFD data;

    data.ctx = websocketStart (port); /**< initialize websocket connection on given port */

    gpioInitialise (); /**< initialize pigpio with standard GPIO numbers */

    //turns off all LEDs
    gpioSetMode ( 4, PI_OUTPUT);
    gpioSetMode ( 4, 1);
    gpioSetMode (17, PI_OUTPUT);
    gpioSetMode (17, 1);
    gpioSetMode (27, PI_OUTPUT);
    gpioSetMode (27, 1);

    gpioSetMode (23, PI_OUTPUT);
    gpioSetMode (23, 1);
    gpioSetMode (22, PI_OUTPUT);
    gpioSetMode (22, 1);
    gpioSetMode (24, PI_OUTPUT);
    gpioSetMode (24, 1);

    gpioSetMode ( 5, PI_OUTPUT);
    gpioSetMode ( 5, 1);
    gpioSetMode (12, PI_OUTPUT);
    gpioSetMode (12, 1);
    gpioSetMode ( 6, PI_OUTPUT);
    gpioSetMode ( 6, 1);

    gpioSetMode (13, PI_OUTPUT);
    gpioSetMode (13, 1);
    gpioSetMode (16, PI_OUTPUT);
    gpioSetMode (16, 1);
    gpioSetMode (26, PI_OUTPUT);
    gpioSetMode (26, 1);

    //turns on red LEDs
    gpioSetMode ( 4, 0);
    gpioSetMode (23, 0);
    gpioSetMode ( 5, 0);
    gpioSetMode (13, 0);

    //calls specific setup routines
    data.ppgFD = ppgSetup (ppgChannel);
    data.ecgFD = ecgSetup (ecgChannel);
    data.tempFD = temperatureSetup (tempChannel);

    //turns off red LEDs; turns on green LEDs
    gpioSetMode (17, 0);
    gpioSetMode ( 4, 1);
    gpioSetMode (22, 0);
    gpioSetMode (23, 1);
    gpioSetMode (12, 0);
    gpioSetMode ( 5, 1);
    gpioSetMode (16, 0);
    gpioSetMode (13, 1);

    return data;
}

int swapBytes(int word){
    return (((word & 0xFF00) >> 8) + ((word & 0x00FF) << 8));
}
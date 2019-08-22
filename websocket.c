/**
 * @file
 * @author  Lars Fischer <uni@larsfischer.name>
 * @version 2.0
 *
 * @section DESCRIPTION
 *
 * This file contains all necessary functions for websocket transmissions.
 */
#include "global.h"

//open websocket server
/**
 * @brief start websocket server
 *
 * This function starts the websocket server, binds port and address. It exits on error.
 *
 * @param port (char*) port which should be used
 * @return websocket context parameter
 */
libwebsock_context *websocketStart (char *port) {
    libwebsock_context *ctx = NULL; /**< variable to take websocket context and return it */

    ctx = libwebsock_init (NULL, NULL, 1024); /**< initialize websocket and save handler */

    if (ctx == NULL) { /**< check for errors */
        fprintf (stderr, "Error during websocket init.\n");
        exit (1);
    }

    libwebsock_bind (ctx, (char *) "0.0.0.0", port); /**< bind to IP address */

    return ctx; /**< return handler */
}

/**
 * @brief send data over websocket
 *
 * This function calls libwebsock's function for sending data to all connected hosts
 *
 * @param ctx (libwebsock_context*) websocket context parameter
 * @param data (char*) data which shall be transmitted to receiver
 * @return number of clients connected
 */
int websocketSend (libwebsock_context *ctx, char *data) {
    return libwebsock_send_all_text (ctx, data);
}

/**
 * @brief wait for websocket data
 *
 * From https://github.com/payden/libwebsock/blob/master/docs/API.txt: "Endless loop function that uses e poll to handle incoming data and process it appropriately.  This function is called last and is what makes libwebsock tick"
 *
 * @param ctx (libwebsock_context*) websocket context parameter
 */
void* websocketWait (void* ctx) {
    libwebsock_wait ((libwebsock_context*) ctx);

    return ctx;
}

/**
  * @brief prepares and serves data from this monitor to frontend via websocket
  *
  * This function is the main websocket handling function. It reads all global variables regularly in an infinite loop and sends the current data in a structured JSON to connected clients.
  *
  * @param ctx `(libwebsock_context*)` websocket context parameter
  * @param ecgData `(dataECG)` ecg data to be transmitted
  * @param ppgData `(dataPPG)` ppg data to be transmitted
  * @param dataTemp `(float)` temperature data to be transmitted
  * @param numberIterations `(int)` number of iterations during which data was collected
  * @param spo2Value `(float)` current spo2 value
  * @param ecgFrequency `(int)` current heart rate
  * @param ppgFrequency `(int)` current pulse rate
  * @param respirationFrequency `(int)` current respiration rate
  */
void websocketSever (libwebsock_context *ctx, dataECG* ecgData, dataPPG* ppgData, float dataTemp, int numberIterations,
                     float spo2Value, int ecgFrequency, int ppgFrequency, int respirationFrequency) {
    char jsonString[numberIterations * 350 * 2]; /**< full JSON "container" */
    char jsonString2[300]; /**< partly JSON "container" for single ECG lead values */

    //generate JSON
    snprintf (jsonString, sizeof jsonString, "{\"valuesECG\": [");
    for (int i = 0; i < numberIterations; i++) {
        snprintf (jsonString2, sizeof jsonString2,
                  "{\"ECGI\": %i, \"ECGII\": %i, \"ECGIII\": %i, \"ECGaVR\": %i, \"ECGaVL\": %i, \"ECGaVF\": %i, \"breathMag\": %i},",
                  ecgData[i].ecgI, ecgData[i].ecgII, ecgData[i].ecgIII, ecgData[i].ecgAVR,
                  ecgData[i].ecgAVL, ecgData[i].ecgAVF, ecgData[i].ecgResp);
        strcat (jsonString, jsonString2);
    }
    jsonString[strlen (jsonString) - 1] = 0; /**< remove last comma from string to add next part of JSON */

    snprintf (jsonString2, sizeof jsonString2, "], \"valuesPPG\": [");
    strcat (jsonString, jsonString2);
    for (int i = 0; i < numberIterations; i++) {
        snprintf (jsonString2, sizeof jsonString2,
                  "{\"ppg1\": %.10f, \"ppg2\": %.10f},", ppgData[i].Hb, ppgData[i].HbO2);
        strcat (jsonString, jsonString2);
    }
    jsonString[strlen (jsonString) - 1] = 0; /**< remove last comma from string to add next part of JSON */

    //generate json part string end part and add it to main json string
    snprintf (jsonString2, sizeof jsonString2,
              "], \"breathFreq\": %d, \"spo2Value\": %.2f, \"heartFreq\": %d, \"pulseFreq\": %d, \"temperature\": %.1f, \"lengthECG\": %d, \"lengthPPG\": %d}",
              respirationFrequency, spo2Value, ecgFrequency, ppgFrequency, dataTemp, numberIterations, numberIterations);
    strcat (jsonString, jsonString2);

    //send data through websocket
    websocketSend (ctx, jsonString);

}
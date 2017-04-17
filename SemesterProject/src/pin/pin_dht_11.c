/**
 * @file pin_dht_11.c
 * @brief DHT_11 function implementaion
 * @author Xiangyu Guo
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>

#include "pin_dht_11.h"

#define MAXTIMINGS      (85)

#define DHT_DATA_PIN    (28)
#define DOWN_TIME       (18)
#define UP_TIME         (40)

#define ONE_BYTE        (8)
#define CHECK_MASK      (0xFF)

static int g_dht_bytes[5] = { 0, 0, 0, 0, 0 };

static int pin_dht_11_inner_read(dht_data_st *data);
/**
 * @brief read data from the module.
 * @param str [out] a valid output buffer.
 * @return 0 success, otherwise ENODATA
 */
int pin_dht_11_read(dht_data_st *data) {
    while (pin_dht_11_inner_read(data) == ENODATA) { 
        printf("Failed to read data\n");       
    }
    return 0;
}

/**
 * @brief read data from the module.
 * @param str [out] a valid output buffer.
 * @return 0 success, otherwise ENODATA
 */
static int pin_dht_11_inner_read(dht_data_st *data)
{
    uint8_t laststate   = HIGH;
    uint8_t counter     = 0;
    uint8_t i, j        = 0;
    int32_t result      = ENODATA;

    if (data == NULL)
        return result;

    memset(g_dht_bytes, 0, sizeof(g_dht_bytes));

    /* pull pin down for 18 milliseconds */
    pinMode(DHT_DATA_PIN, OUTPUT);
    digitalWrite(DHT_DATA_PIN, LOW);
    delay(DOWN_TIME);
    /* then pull it up for 40 microseconds */
    digitalWrite(DHT_DATA_PIN, HIGH);
    delayMicroseconds(UP_TIME);
    /* prepare to read the pin */
    pinMode(DHT_DATA_PIN, INPUT);

    /* detect change and read data */
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_DATA_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == CHECK_MASK)
                break;
        }

        if (counter == CHECK_MASK)
            break;

        laststate = digitalRead(DHT_DATA_PIN);

        /* ignore first 3 transitions */
        if (i < 3)
            continue;

        if ((i % 2 == 0)) {
            /* shove each bit into the storage bytes */
            g_dht_bytes[j / ONE_BYTE] <<= 1;
            if (counter > 16)
                g_dht_bytes[j / ONE_BYTE] |= 1;
            j++;
        }
    }

    /*
     * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     * print it out if data is good
     */
    if ((j >= 40) &&
        (g_dht_bytes[4] == ((g_dht_bytes[0] + g_dht_bytes[1] + 
                           g_dht_bytes[2] + g_dht_bytes[3]) & CHECK_MASK))) {
        data->humidity = g_dht_bytes[0];
        data->humidity += g_dht_bytes[1] / 10.0;

        data->temperature = g_dht_bytes[2];
        data->temperature += g_dht_bytes[3] / 10.0;

        printf( "Humidity = %d.%d %% Temperature = %d.%d *C\n",
            g_dht_bytes[0], g_dht_bytes[1], g_dht_bytes[2], g_dht_bytes[3]);
        result = 0;
    } else {
        printf( "Data not good, skip\n" );
        result = ENODATA;
    }

    return result;
}

/**
 * @brief initialize the module.
 */
void pin_dht_11_init() {
    if (wiringPiSetup() == -1)
        exit(errno);
}

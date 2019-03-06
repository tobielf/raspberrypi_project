/**
 * @file web_server.c
 * @brief implementation of web server.
 * @author Xiangyu Guo
 */
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include <wiringPi.h>

#include "spi/spi_mcp3208.h"
#include "i2c/i2c_bmp180.h"
#include "pin/pin_motor.h"
#include "pin/pin_dht_11.h"

#include "web_server.h"

#define MAX_LIGHT_BOUNDRY       (4)     /**< LED from 0 - 4, 5 in total. */

#define LED_STATUS_THRESHOLD    (100)   /**< LED off status */

#define POWER_PIN               (24)    /**< wiringPi pin number of power */

const int g_led_pins[MAX_LIGHT_BOUNDRY + 1] = {7, 0, 2, 3, 25}; /**< LED on GPIO*/

/**
 * ===========================================
 * Call back functions handle the http request
 * ===========================================
 */
static void power_request_cb(struct evhttp_request *req, void *arg);

static void switch_request_cb(struct evhttp_request *req, void *arg);

static void status_request_cb(struct evhttp_request *req, void *arg);

static void temperature_request_cb(struct evhttp_request *req, void *arg);

static void temp_humi_request_cb(struct evhttp_request *req, void *arg);

static void dump_request_cb(struct evhttp_request *req, void *arg);

/**
 * @brief setup the wiringPi
 */
static void setup_wiringPi(void);

/**
 * @brief setting up the web server
 * @param base event base.
 */
void web_server_init(struct event_base *base) {
    struct evhttp *http;
    struct evhttp_bound_socket *handle;

    ev_uint16_t port = 80;

    setup_wiringPi();

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) {
        fprintf(stderr, "couldn't create evhttp. Exiting.\n");
        exit(errno);
    }

    evhttp_set_cb(http, "/power/on", power_request_cb, "on");

    evhttp_set_cb(http, "/power/off", power_request_cb, "off");

    evhttp_set_cb(http, "/power/status", power_request_cb, "status");
    
    //evhttp_set_cb(http, "/status", status_request_cb, NULL);

    //evhttp_set_cb(http, "/switch/on", switch_request_cb, "on");

    //evhttp_set_cb(http, "/switch/off", switch_request_cb, "off");

    //evhttp_set_cb(http, "/motor/on", switch_request_cb, "on");

    //evhttp_set_cb(http, "/motor/off", switch_request_cb, "off");

    //evhttp_set_cb(http, "/temp/status", temperature_request_cb, NULL);

    evhttp_set_cb(http, "/temp_humi/status", temp_humi_request_cb, NULL);

    /* The /dump URI will dump all requests to stdout and say 200 ok. */
    evhttp_set_gencb(http, dump_request_cb, NULL);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if (!handle) {
        fprintf(stderr, "couldn't bind to port %d. Exiting.\n", (int)port);
        exit(errno);
    }
    printf("server started\n");
}

static void
power_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;

    if (strcmp(arg, "on") == 0) {
        digitalWrite(POWER_PIN, 1);
    } else if (strcmp(arg, "off") == 0) {
        digitalWrite(POWER_PIN, 0);
    } else {
        evb = evbuffer_new();
        evbuffer_add_printf(evb, "%d\n", digitalRead(POWER_PIN));
    }
    evhttp_send_reply(req, 200, "OK", NULL);
    if (evb != NULL)
        evbuffer_free(evb);
}

static void
switch_request_cb(struct evhttp_request *req, void *arg)
{
    struct evkeyvalq headers;
    const char *q;
    int led;
    // Parse the query for later lookups
    evhttp_parse_query(evhttp_request_get_uri(req), &headers);

    q = evhttp_find_header (&headers, "led");
    led = atoi(q);

    if (strcmp(arg, "on")) {
        digitalWrite(g_led_pins[led], 0);
    } else {
        digitalWrite(g_led_pins[led], 1);
    }
    evhttp_send_reply(req, 200, "OK", NULL);
}

static void
status_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    struct evkeyvalq headers;
    mcp3208_module_st *mcp3208 = NULL;
    const char *q;
    // Parse the query for later lookups
    evhttp_parse_query(evhttp_request_get_uri(req), &headers);

    q = evhttp_find_header (&headers, "led");

    mcp3208 = mcp3208_module_get_instance();
    int value = mcp3208_read_data(mcp3208, atoi(q));

    evhttp_request_get_uri(req);
    evb = evbuffer_new();
    if (value < LED_STATUS_THRESHOLD)
        evbuffer_add_printf(evb, "0\n");
    else
        evbuffer_add_printf(evb, "1\n");

    evhttp_send_reply(req, 200, "OK", evb);
    evbuffer_free(evb);
}

static void
temperature_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    bmp180_module_st *bmp180 = NULL;
    bmp180_data_st value;

    bmp180 = bmp180_module_init(BMP180_ULTRA_HIGH_RESOLUTION);
    bmp180_read_data(bmp180, &value);
    evb = evbuffer_new();
    evbuffer_add_printf(evb, "{\"temperature\": %.1f, \"humidity\": 0}", 
                        value.temperature);
    evhttp_send_reply(req, 200, "OK", evb);
    bmp180_module_fini(bmp180);
    evbuffer_free(evb);
}

static void
temp_humi_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    dht_data_st value;

    pin_dht_11_init();
    pin_dht_11_read(&value);

    evb = evbuffer_new();
    evbuffer_add_printf(evb, "{\"temperature\": %.2f, \"humidity\": %.2f}", 
                        value.temperature, value.humidity);
    evhttp_send_reply(req, 200, "OK", evb);

    evbuffer_free(evb);
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
dump_request_cb(struct evhttp_request *req, void *arg)
{
    const char *cmdtype;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    struct evbuffer *buf;

    switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_GET: cmdtype = "GET"; break;
    case EVHTTP_REQ_POST: cmdtype = "POST"; break;
    case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
    case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
    case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
    case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
    case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
    case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
    case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
    default: cmdtype = "unknown"; break;
    }

    printf("Received a %s request for %s\nHeaders:\n",
        cmdtype, evhttp_request_get_uri(req));

    headers = evhttp_request_get_input_headers(req);
    for (header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        printf("  %s: %s\n", header->key, header->value);
    }

    buf = evhttp_request_get_input_buffer(req);
    puts("Input data: <<<");
    while (evbuffer_get_length(buf)) {
        int n;
        char cbuf[128];
        n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
        if (n > 0)
            (void) fwrite(cbuf, 1, n, stdout);
    }
    puts(">>>");

    evhttp_send_reply(req, 200, "OK", NULL);
}

static void
setup_wiringPi(void) {
    int i, led;

    if (wiringPiSetup() < 0)
        exit(errno);

    for (i = 0; i <= MAX_LIGHT_BOUNDRY; ++i) {
        led = g_led_pins[i];
        pinMode(led, OUTPUT);
        digitalWrite(led, 0);
    }
}

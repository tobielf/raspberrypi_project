/**
 * @file smarthomed.c
 * @brief Smart Home
 *      1. Alarm System (Motion detector, Mecury switch, MCP3208 ADC[Brightness, Threshold])
 *      2. Screen Display (Update display)
 *      3. Temperature setup_motor_event(base);
 *      4. Web Server for Siri. (LED lights, MCP3208 ADC[status])
    web_server_init(base);
 * @author Xiangyu Guo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

#include <event2/event.h>
#include <event2/http.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <wiringPi.h>

#include "spi/spi_mcp3208.h"
#include "i2c/i2c_bmp180.h"
#include "i2c/i2c_lcd1620.h"
#include "pin/pin_motor.h"
#include "pin/pin_dht_11.h"

#include "screen.h"
#include "web_server.h"

#define MOTION_DETECTOR     (29)        /**< wiringPi pin number of motion detector */
#define MECURY_SWITCH       (27)        /**< wiringPi pin number of mecury switch */
#define ALARM_LIGHT         (24)        /**< wiringPi pin number of alarm light */

#define INTERRUPT_INTERVAL  (200)       /**< Bouncing Interval */

#define TIMEOUT_SEC         (3)         /**< Temperature Event Time out */

#define TEMPERATURE_LOWEST  (10)        /**< Lowest temperature to turn on the fan */
#define TEMPERATURE_RANGE   (30)        /**< Temperature range conver from ADC */

void reqcb(struct evhttp_request * req, void * arg)
{
    printf("Send one request\n");
}

static int s_motion_fd = -1;

/**
 * @brief Interrupt handler
 */
void isr_motion_detector (void) {
    int brightness;
    int brightness_threshold;
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    // mcp3208_module_st *mcp3208 = NULL;
    
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL && s_motion_fd == -1) {
        // mcp3208 = mcp3208_module_get_instance();

        // Receive the brightness (Ch5)
        // brightness = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_5);

        // Compare with the threshold (Ch6)
        // brightness_threshold = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_6);

        //printf("Mercury Switcher: %d\nBrightness: %d\nBrightness threshold: %d\n", 
        //        digitalRead(MECURY_SWITCH), brightness, brightness_threshold);
        // Check with Mercury Switcher.
        //if (digitalRead(MECURY_SWITCH) == 0 && brightness < brightness_threshold) {
        //    digitalWrite(ALARM_LIGHT, 1);
        //} else {
        //    digitalWrite(ALARM_LIGHT, 0);
        //}
        s_motion_fd = 1;

        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void setup_alram_system() {

    if (wiringPiSetup() < LOW)
        exit(errno);

    // pinMode(MECURY_SWITCH, INPUT);
    pinMode(ALARM_LIGHT, OUTPUT);

    if (wiringPiISR(MOTION_DETECTOR, INT_EDGE_FALLING, &isr_motion_detector) < 0)
        exit(errno);
}

static void 
update_display_callback(evutil_socket_t fd, short flags, void *data) {
    screen_update_display();
}

static void 
check_temperature_callback(evutil_socket_t fd, short flags, void *data) {
    mcp3208_module_st *mcp3208 = mcp3208_module_get_instance();
    bmp180_module_st *bmp180 = bmp180_module_init(BMP180_ULTRA_HIGH_RESOLUTION);
    bmp180_data_st value;
    double temperature_threshold = 0;

    bmp180_read_data(bmp180, &value);
    
    // Read temerpature thresh_hold (Ch7).
    temperature_threshold = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_7);
    temperature_threshold = temperature_threshold / MCP3208_MAX_VALUE *
                                     TEMPERATURE_RANGE + TEMPERATURE_LOWEST;

    printf("====Temperature====\n");
    printf("Current:%.2f\n", value.temperature);
    printf("Setting:%.2f\n", temperature_threshold);
    if (value.temperature > temperature_threshold)
        motor_turn_on();
    else
        motor_turn_off();

    bmp180_module_fini(bmp180);
}

static void on_connection_close(struct evhttp_connection* connection, void* arg) {
    fprintf(stderr, "remote connection closed\n");
    //event_base_loopexit((struct event_base*)arg, NULL);
}

void on_request_close(struct evhttp_request* rsp, void* arg)
{
} 

int on_header_respond(struct evhttp_request* rsp, void* arg)
{
    fprintf(stderr, "< HTTP/1.1 %d %s\n", evhttp_request_get_response_code(rsp), evhttp_request_get_response_code_line(rsp));
    struct evkeyvalq* headers = evhttp_request_get_input_headers(rsp);
    struct evkeyval* header;
    fprintf(stderr, "< \n");
    return 0;
}

void on_chunk_data(struct evhttp_request* rsp, void* arg)
{
    char buf[4096];
    struct evbuffer* evbuf = evhttp_request_get_input_buffer(rsp);
    int n = 0;
    while ((n = evbuffer_remove(evbuf, buf, 4096)) > 0)
    {
        fwrite(buf, n, 1, stdout);
    }
}

void on_request_error(enum evhttp_request_error error, void* arg)
{
    fprintf(stderr, "request failed\n");
    event_base_loopexit((struct event_base*)arg, NULL);
}

static void motion_detect_callback(evutil_socket_t fd, short flags, void *data) {
    if (s_motion_fd == -1)
        return;

    s_motion_fd = -1;

    printf("====Event: Motion====\n");

    struct event_base *base = (struct event_base *) data;

    char *url = "http://10.0.1.200:18089";
    struct evhttp_uri* uri = evhttp_uri_parse(url);

    struct evhttp_connection *conn;
    struct evhttp_request *req;

    const char* host = evhttp_uri_get_host(uri);
    int port = evhttp_uri_get_port(uri);
    
    conn = evhttp_connection_base_new(base, NULL, host, port);

    evhttp_connection_set_closecb(conn, on_connection_close, base);
    req = evhttp_request_new(on_request_close, base);

    evhttp_request_set_header_cb(req, on_header_respond);
    evhttp_request_set_chunked_cb(req, on_chunk_data);
    evhttp_request_set_error_cb(req, on_request_error);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Host", host);
    //evhttp_add_header(req->output_headers, "Connection", "close");

    //evhttp_connection_set_timeout(req->evcon, 600);
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/");
    //printf("evhttp_make_request: %i\n",
    //       evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/"));
}

static void setup_motor_event(struct event_base *base) {
    struct timeval tv;
    struct event *temperature_check_event;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    temperature_check_event = event_new(base, -1, 
                EV_TIMEOUT | EV_PERSIST, check_temperature_callback, NULL);

    if (!temperature_check_event) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        exit(errno);
    }
    event_add(temperature_check_event, &tv);

    motor_setup_up();
}

static void setup_update_event(struct event_base *base) {
    struct timeval tv;
    struct event *update_display_event;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    update_display_event = event_new(base, -1, 
                EV_TIMEOUT | EV_PERSIST, update_display_callback, NULL);

    if (!update_display_event) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        exit(errno);
    }
    event_add(update_display_event, &tv);
}

static void setup_motion_event(struct event_base *base) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500;
    struct event *motion_event = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, motion_detect_callback, base);

    event_add(motion_event, &tv);
}

int main(int argc, char **argv)
{
    struct event_base *base;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return errno;

    setup_alram_system();

    //screen_display_get_instance();

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        return 1;
    }

    //setup_motor_event(base);

    //setup_update_event(base);

    setup_motion_event(base);

    web_server_init(base);

    event_base_dispatch(base);

    //mcp3208_module_clean_up();

    return 0;
}

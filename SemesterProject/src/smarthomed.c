#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <wiringPi.h>

#include "spi/spi_mcp3208.h"
#include "i2c/i2c_bmp180.h"
#include "pin/pin_motor.h"
#include "pin/pin_dht_11.h"

#define MAX_LIGHT_BOUNDRY   (4)     // LED from 0 - 7, 8 in total.

#define STARTSTOP_BUTTON    (6)     // Using GPIO 16 as start/stop control
#define MOTION_DETECTOR     (29)
#define MECURY_SWITCH       (27)

#define INTERRUPT_INTERVAL  200     // Bouncing Interval

char uri_root[512];

const int g_led_pins[MAX_LIGHT_BOUNDRY + 1] = {7, 0, 2, 3, 25};

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

    mcp3208 = mcp3208_module_init(0, 100000);
    int value = mcp3208_read_data(mcp3208, atoi(q));

    evhttp_request_get_uri(req);
    evb = evbuffer_new();
    if (value < 100)
        evbuffer_add_printf(evb, "0\n");
    else
        evbuffer_add_printf(evb, "1\n");
    evhttp_send_reply(req, 200, "OK", evb);
    mcp3208_module_fini(mcp3208);
    evbuffer_free(evb);
}

static void
temperature_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    bmp180_module_st *bmp180 = NULL;
    bmp180_data_st value;

    bmp180 = bmp180_module_init(3);
    bmp180_read_data(bmp180, &value);
    evb = evbuffer_new();
    evbuffer_add_printf(evb, "{\"temperature\": %.1f, \"humidity\": 38}", value.temperature);
    evhttp_send_reply(req, 200, "OK", evb);
    bmp180_module_fini(bmp180);
    evbuffer_free(evb);
}

static void
temp_humi_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    char str[255];

    pin_dht_11_init();
    pin_dht_11_read(str);
    evb = evbuffer_new();
    evbuffer_add_printf(evb, "%s", str);
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
syntax(void)
{
    fprintf(stdout, "Syntax: http-server <docroot>\n");
}

static void
setupWiringPi(void) {
    int i, led;

    if (wiringPiSetup() < 0)
        exit(errno);

    for (i = 0; i <= MAX_LIGHT_BOUNDRY; ++i) {
        led = g_led_pins[i];
        pinMode(led, OUTPUT);
        digitalWrite(led, 0);
    }
}

/**
 * @brief Interrupt handler
 */
void isrStartStopButton (void) {
    static unsigned long s_last_interrupt = 0;
    static int s_started = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        // Start or Stop
        s_started = (s_started + 1) & 1;
        if (s_started)
            motor_turn_on();
        else
            motor_turn_off();
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

/**
 * @brief Interrupt handler
 */
void isrChangeDirectionButton (void) {
    static unsigned long s_last_interrupt = 0;
    static int s_started = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        // Start or Stop
        s_started = (s_started + 1) & 1;
        if (s_started)
            motor_turn_on();
        else
            motor_turn_off();
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void 
setupMotor() {
    motor_setup_up();

    if (wiringPiISR(STARTSTOP_BUTTON, INT_EDGE_FALLING, &isrStartStopButton) < 0)
        exit(errno);

    //if (wiringPiISR(MOTION_DETECTOR, INT_EDGE_FALLING, &isrChangeDirectionButton) < 0)
    //    exit(errno);

    //if (wiringPiISR(MECURY_SWITCH, INT_EDGE_FALLING, &isrChangeDirectionButton) < 0)
    //    exit(errno);
}

int
main(int argc, char **argv)
{
    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;

    ev_uint16_t port = 80;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return errno;

    if (argc < 2) {
        syntax();
        return 1;
    }

    setupWiringPi();
    setupMotor();

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        return 1;
    }

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) {
        fprintf(stderr, "couldn't create evhttp. Exiting.\n");
        return 1;
    }

    /* The /dump URI will dump all requests to stdout and say 200 ok. */
    evhttp_set_cb(http, "/status", status_request_cb, NULL);

    evhttp_set_cb(http, "/switch/on", switch_request_cb, "on");

    evhttp_set_cb(http, "/switch/off", switch_request_cb, "off");

    evhttp_set_cb(http, "/motor/on", switch_request_cb, "on");

    evhttp_set_cb(http, "/motor/off", switch_request_cb, "off");

    evhttp_set_cb(http, "/temp/status", temperature_request_cb, NULL);

    evhttp_set_cb(http, "/temp_humi/status", temp_humi_request_cb, NULL);

    /* We want to accept arbitrary requests, so we need to set a "generic"
     * cb.  We can also add callbacks for specific paths. */
    evhttp_set_gencb(http, dump_request_cb, NULL);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if (!handle) {
        fprintf(stderr, "couldn't bind to port %d. Exiting.\n",
            (int)port);
        return 1;
    }

    {
        /* Extract and display the address we're listening on. */
        struct sockaddr_storage ss;
        evutil_socket_t fd;
        ev_socklen_t socklen = sizeof(ss);
        char addrbuf[128];
        void *inaddr;
        const char *addr;
        int got_port = -1;
        fd = evhttp_bound_socket_get_fd(handle);
        memset(&ss, 0, sizeof(ss));
        if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
            perror("getsockname() failed");
            return 1;
        }
        if (ss.ss_family == AF_INET) {
            got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
            inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
        } else if (ss.ss_family == AF_INET6) {
            got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
            inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
        } else {
            fprintf(stderr, "Weird address family %d\n",
                ss.ss_family);
            return 1;
        }
        addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
            sizeof(addrbuf));
        if (addr) {
            printf("Listening on %s:%d\n", addr, got_port);
            evutil_snprintf(uri_root, sizeof(uri_root),
                "http://%s:%d",addr,got_port);
        } else {
            fprintf(stderr, "evutil_inet_ntop failed\n");
            return 1;
        }
    }

    event_base_dispatch(base);

    return 0;
}
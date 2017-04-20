/**
 * @file web_server.h
 * @brief interface definition of web server.
 * @author Xiangyu Guo
 */
#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

/**
 * @brief setting up the web server
 * @param base event base.
 * @return 0 on success, otherwise errno
 */
int web_server_init(struct event_base *base);

#endif
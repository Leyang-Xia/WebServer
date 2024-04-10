#ifndef _LIBEVENT_HTTP_H
#define _LIBEVENT_HTTP_H
#include "headfile.h"

void run_http_server(int port);

void conn_eventcb(struct bufferevent *bev, short events, void *user_data);

void conn_readcb(struct bufferevent *bev, void *user_data);

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                 struct sockaddr *sa, int socklen, void *user_data);

void signal_cb(evutil_socket_t sig, short events, void *user_data);

#endif
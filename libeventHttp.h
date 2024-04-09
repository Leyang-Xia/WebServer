#ifndef _LIBEVENT_HTTP_H
#define _LIBEVENT_HTTP_H

#include <event2/event.h>
class Http {
public:
    std::string httpMessage;
    std::string method;
    std::string path;
    std::string protocol;
    Http(const char* buffer);
    Http(Http&& other) noexcept;
    ~Http() {};
};




void run_http_server(int port);

void conn_eventcb(struct bufferevent *bev, short events, void *user_data);

void conn_readcb(struct bufferevent *bev, void *user_data);

const char *get_file_type(char *name);

int hexit(char c);

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                 struct sockaddr *sa, int socklen, void *user_data);

int response_http(struct bufferevent *bev, const std::string& method, const std::string& path);

int send_dir(struct bufferevent *bev,const char *dirname);

int send_error(struct bufferevent *bev);

int send_file_to_http(const char *filename, struct bufferevent *bev);

int send_header(struct bufferevent *bev, int no, const char* desp, const char *type, long len);

void signal_cb(evutil_socket_t sig, short events, void *user_data);

void strdecode(char *to, char *from);

void strencode(char* to, size_t tosize, const char* from);

#endif
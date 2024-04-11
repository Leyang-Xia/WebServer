#include "libeventHttp.h"
#include "http.h"
//#include "ThreadPool.h"
std::mutex mutex;
void run_http_server(int port) {
    struct event_base *base;
    struct evconnlistener *listener;
    struct event *signal_event;

    struct sockaddr_in sin;
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    // 创建监听的套接字，绑定，监听，接受连接请求
    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_DEFERRED_ACCEPT, -1,
                                       (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return;
    }

    // 创建信号事件, 捕捉并处理
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    if (!signal_event || event_add(signal_event, nullptr)<0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return;
    }

    // 事件循环
    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);
}

void listener_cb(struct evconnlistener *listener,
                 evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
    std::thread thread([&]() {
//        std::unique_lock<std::mutex> lock(mutex);
        printf("******************** begin call-------%s\n",__FUNCTION__);
        struct event_base *base = static_cast<event_base *>(user_data);
        struct bufferevent *bev;
        printf("fd is %d\n",fd);

        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

        if (!bev)
        {
            fprintf(stderr, "Error constructing bufferevent!");
            event_base_loopbreak(base);
            return;
        }
//        lock.unlock();
        // 设置读取超时时间为 10 秒
        struct timeval timeout = {1800, 0};
        bufferevent_set_timeouts(bev, &timeout, nullptr);

        bufferevent_flush(bev, EV_READ | EV_WRITE, BEV_NORMAL);
        bufferevent_setcb(bev, conn_readcb, nullptr, conn_eventcb, nullptr);
        bufferevent_enable(bev, EV_READ | EV_WRITE);

        printf("******************** end call-------%s\n",__FUNCTION__);
    });
    thread.detach();
}


void conn_readcb(struct bufferevent *bev, void *user_data)
{
    printf("******************** begin call %s.........\n",__FUNCTION__);

//    char method[50], path[4096], protocol[32];
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    char buf[len];
    bufferevent_read(bev, buf, sizeof(buf));
    std::unique_ptr<Http> http(new Http(buf));

//    printf("buf[%s]\n", buf);
//    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
//    printf("method[%s], path[%s], protocol[%s]\n", method, path, protocol);

    if( "GET" == http->method)
    {
        http->response_http(bev);
    }
    printf("******************** end call %s.........\n", __FUNCTION__);
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    printf("******************** begin call %s.........\n", __FUNCTION__);
    if (events & BEV_EVENT_EOF)
    {
        printf("Connection closed.\n");
    }
    else if (events & BEV_EVENT_ERROR)
    {
        printf("Got an error on the connection: %s\n",
               strerror(errno));
    } else if (events & BEV_EVENT_TIMEOUT) {
        printf("Connection timeout.\n");
    }

    bufferevent_free(bev);

    printf("******************** end call %s.........\n", __FUNCTION__);
}

void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = static_cast<event_base *>(user_data);
    struct timeval delay = { 1, 0 };

    printf("Caught an interrupt signal; exiting cleanly in one seconds.\n");
    event_base_loopexit(base, &delay);
}



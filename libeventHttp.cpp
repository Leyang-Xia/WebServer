#include <winsock.h>
#include "libeventHttp.h"
#include "http.h"
#include "ThreadPool.h"
#include <vector>
#include <thread>
#include <mutex>
#include <event2/event.h>
#include <event2/listener.h>

std::mutex mutex;
std::vector<struct event_base*> bases;
std::vector<std::thread> threads;
std::mutex base_mutex;
int next_base = 0;

void event_base_loop(struct event_base* base) {
    event_base_dispatch(base);
}

void init_event_bases(int num_bases) {
    for (int i = 0; i < num_bases; ++i) {
        struct event_base* base = event_base_new();
        bases.push_back(base);
        threads.emplace_back(event_base_loop, base);
    }
}


void run_http_server(int port, int num_bases) {
    struct event_base *main_base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    main_base = event_base_new();
    if (!main_base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return;
    }

    init_event_bases(num_bases);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    listener = evconnlistener_new_bind(main_base, listener_cb, nullptr,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_DEFERRED_ACCEPT, -1,
                                       (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return;
    }

    event_base_dispatch(main_base);

    evconnlistener_free(listener);
    event_base_free(main_base);

    for (auto& thread : threads) {
        thread.join();
    }

    for (auto& base : bases) {
        event_base_free(base);
    }
}

// void process_in_new_thread(int fd) {
//     if (fd < 0) {
//         std::cout << "process_in_new_thread_when_accepted() quit!" << std::endl;
//         return;
//     }

//     // 初始化base,写事件和读事件
//     struct event_base* base = event_base_new();
//     struct bufferevent *bev;
//     printf("fd is %d\n", fd);

//     bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

//     if (!bev) {
//         fprintf(stderr, "Error constructing bufferevent!");
//         event_base_loopbreak(base);
//         return;
//     }

//     struct timeval timeout = {1800, 0};
//     bufferevent_set_timeouts(bev, &timeout, nullptr);

//     bufferevent_flush(bev, EV_READ | EV_WRITE, BEV_NORMAL);
//     bufferevent_setcb(bev, conn_readcb, nullptr, conn_eventcb, nullptr);
//     bufferevent_enable(bev, EV_READ | EV_WRITE);

//     printf("******************** end call-------%s\n", __FUNCTION__);

//     // 开始libevent的loop循环
//     event_base_dispatch(base);
//     return;
// }


void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data) {
    std::lock_guard<std::mutex> lock(base_mutex);
    struct event_base* base = bases[next_base];
    next_base = (next_base + 1) % bases.size();

    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        close(fd);
        return;
    }

    bufferevent_setcb(bev, conn_readcb, nullptr, conn_eventcb, nullptr);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
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
        bufferevent_free(bev);
    }

    printf("******************** end call %s.........\n", __FUNCTION__);
}

void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = static_cast<event_base *>(user_data);
    struct timeval delay = { 1, 0 };

    printf("Caught an interrupt signal; exiting cleanly in one seconds.\n");
    event_base_loopexit(base, &delay);
}






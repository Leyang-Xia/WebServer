#include "libeventHttp.h"
//#include "ThreadPool.h"
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
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
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

    printf("done\n");
}

int response_http(struct bufferevent *bev, const std::string& method, const std::string& path) {
    if (strcasecmp("GET", method.c_str()) == 0) {
        // get method ...
        char decoded_path[path.size() + 1];
        strcpy(decoded_path, path.c_str());
        strdecode(decoded_path, decoded_path);
        char *pf = &decoded_path[1];

        if (path == "/" || path == "/.") {
            pf = const_cast<char*>("./");
        }

        printf("***** http Request Resource Path =  %s, pf = %s\n", decoded_path, pf);

        struct stat sb;
        if (stat(pf, &sb) < 0) {
            perror("open file err:");
            send_error(bev);
            return -1; // 返回-1表示出错
        }

        if (S_ISDIR(sb.st_mode)) {
            // 处理目录
            send_header(bev, 200, "OK", get_file_type(".html"), -1);
            send_dir(bev, pf);
        } else {
            // 处理文件
            send_header(bev, 200, "OK", get_file_type(pf), sb.st_size);
            send_file_to_http(pf, bev);
        }
    } else {
        // 其他请求方法可以在这里处理，如POST、PUT等
        // 目前只处理GET请求，对于其他请求返回405 Method Not Allowed
        send_header(bev, 405, "Method Not Allowed", "text/plain", -1);
        const char *msg = "Method Not Allowed";
        bufferevent_write(bev, msg, strlen(msg));
    }

    // 不要在这里返回，让服务器保持连接并继续处理其他请求
    // return 0;
}


/*
     *charset=iso-8859-1	西欧的编码，说明网站采用的编码是英文；
     *charset=gb2312		说明网站采用的编码是简体中文；
     *charset=utf-8			代表世界通用的语言编码；
     *						可以用到中文、韩文、日文等世界上所有语言编码上
     *charset=euc-kr		说明网站采用的编码是韩文；
     *charset=big5			说明网站采用的编码是繁体中文；
     *
     *以下是依据传递进来的文件名，使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
*/
const char *get_file_type(char *name)
{
    char* dot;

    dot = strrchr(name, '.');	//自右向左查找‘.’字符;如不存在返回NULL

    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

int send_file_to_http(const char *filename, struct bufferevent *bev)
{
    int fd = open(filename, O_RDONLY);
    int ret = 0;
    char buf[4096] = {0};

    while((ret = read(fd, buf, sizeof(buf)) ) )
    {
        bufferevent_write(bev, buf, ret);
        memset(buf, 0, ret);
    }
    close(fd);
    return 0;
}

int send_header(struct bufferevent *bev, int no, const char* desp, const char *type, long len)
{
    char buf[256]={0};

    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desp);
    //HTTP/1.1 200 OK\r\n
    bufferevent_write(bev, buf, strlen(buf));
    // 文件类型
    sprintf(buf, "Content-Type:%s\r\n", type);
    bufferevent_write(bev, buf, strlen(buf));
    // 文件大小
    sprintf(buf, "Content-Length:%ld\r\n", len);
    bufferevent_write(bev, buf, strlen(buf));
    // Connection: close
    bufferevent_write(bev, _HTTP_CLOSE_, strlen(_HTTP_CLOSE_));
    //send \r\n
    bufferevent_write(bev, "\r\n", 2);

    return 0;
}

int send_error(struct bufferevent *bev)
{
    const char* html404 =
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "<head>\n"
            "    <meta charset=\"UTF-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            "    <title>404 Not Found</title>\n"
            "    <style>\n"
            "        body {\n"
            "            font-family: Arial, sans-serif;\n"
            "            background-color: #f4f4f4;\n"
            "            color: #333;\n"
            "            margin: 0;\n"
            "            padding: 0;\n"
            "        }\n"
            "        .container {\n"
            "            max-width: 800px;\n"
            "            margin: 100px auto;\n"
            "            text-align: center;\n"
            "        }\n"
            "        h1 {\n"
            "            font-size: 36px;\n"
            "            margin-bottom: 20px;\n"
            "        }\n"
            "        p {\n"
            "            font-size: 18px;\n"
            "            margin-bottom: 20px;\n"
            "        }\n"
            "        a {\n"
            "            color: #007bff;\n"
            "            text-decoration: none;\n"
            "        }\n"
            "        a:hover {\n"
            "            text-decoration: underline;\n"
            "        }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "<div class=\"container\">\n"
            "    <h1>404 Not Found</h1>\n"
            "    <p>The requested URL was not found on this server.</p>\n"
            "    <p>Go back to <a href=\"/\">homepage</a>.</p>\n"
            "</div>\n"
            "</body>\n"
            "</html>\n";

    send_header(bev, 404, "Not Found", "text/html", strlen(html404));
    bufferevent_write(bev, html404, strlen(html404));

    return 0;
}


int send_dir(struct bufferevent *bev,const char *dirname)
{
    char encoded_name[1024];
    char path[1024];
    char timestr[64];
    struct stat sb;
    struct dirent **dirinfo;
    int i;

    char buf[4096] = {0};
    sprintf(buf, "<html><head><meta charset=\"utf-8\"><title>%s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>当前目录：%s</h1><table>", dirname);
    //添加目录内容
    int num = scandir(dirname, &dirinfo, nullptr, alphasort);
    for(i=0; i<num; ++i)
    {
        // 编码
        strencode(encoded_name, sizeof(encoded_name), dirinfo[i]->d_name);

        sprintf(path, "%s%s", dirname, dirinfo[i]->d_name);
        printf("############# path = %s\n", path);
        if (lstat(path, &sb) < 0)
        {
            sprintf(buf+strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td></tr>\n",
                    encoded_name, dirinfo[i]->d_name);
        }
        else
        {
            strftime(timestr, sizeof(timestr),
                     "  %d  %b   %Y  %H:%M", localtime(&sb.st_mtime));
            if(S_ISDIR(sb.st_mode))
            {
                sprintf(buf+strlen(buf),
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%s</td><td>%ld</td></tr>\n",
                        encoded_name, dirinfo[i]->d_name, timestr, sb.st_size);
            }
            else
            {
                sprintf(buf+strlen(buf),
                        "<tr><td><a href=\"%s\">%s</a></td><td>%s</td><td>%ld</td></tr>\n",
                        encoded_name, dirinfo[i]->d_name, timestr, sb.st_size);
            }
        }
        bufferevent_write(bev, buf, strlen(buf));
        memset(buf, 0, sizeof(buf));
    }
    sprintf(buf+strlen(buf), "</table></body></html>");
    bufferevent_write(bev, buf, strlen(buf));
    printf("################# Dir Read OK !!!!!!!!!!!!!!\n");

    return 0;
}

void conn_readcb(struct bufferevent *bev, void *user_data)
{
    printf("******************** begin call %s.........\n",__FUNCTION__);

    char buf[4096]={0};
//    char method[50], path[4096], protocol[32];

    bufferevent_read(bev, buf, sizeof(buf));
    std::unique_ptr<Http> http(new Http(buf));

//    printf("buf[%s]\n", buf);
//    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
//    printf("method[%s], path[%s], protocol[%s]\n", method, path, protocol);

    if( "GET" == http->method)
    {
        response_http(bev, http->method, http->path);
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

void listener_cb(struct evconnlistener *listener,
                 evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
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
    bufferevent_flush(bev, EV_READ | EV_WRITE, BEV_NORMAL);
    bufferevent_setcb(bev, conn_readcb, nullptr, conn_eventcb, nullptr);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    printf("******************** end call-------%s\n",__FUNCTION__);
}

/*
 * 这里的内容是处理%20之类的东西！是"解码"过程。
 * %20 URL编码中的‘ ’(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * 相关知识html中的‘ ’(space)是&nbsp
 */
void strdecode(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 依次判断from中 %20 三个字符
            *to = hexit(from[1])*16 + hexit(from[2]);
            // 移过已经处理的两个字符(%21指针指向1),表达式3的++from还会再向后移一个字符
            from += 2;
        }
        else
        {
            *to = *from;
        }
    }
    *to = '\0';
}

//16进制数转化为10进制, return 0不会出现
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// "编码"，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
// strencode(encoded_name, sizeof(encoded_name), name);
void strencode(char* to, size_t tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0)
        {
            *to = *from;
            ++to;
            ++tolen;
        }
        else
        {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}


Http::Http(const char* buffer) {
    httpMessage = std::string(buffer);
    std::stringstream ss(httpMessage);
    ss >> method >> path >> protocol;
    std::cout << "method: " << method << ", path: " << path << ", protocol: " << protocol << std::endl;
    buffer = nullptr;
}

// 移动构造函数
Http::Http(Http&& other) noexcept
: httpMessage(std::move(other.httpMessage)), method(std::move(other.method)), path(std::move(other.path)), protocol(std::move(other.protocol)) {
// 清空other中的成员变量
other.method.clear();
other.path.clear();
other.protocol.clear();
}


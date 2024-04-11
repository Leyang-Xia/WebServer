#include "http.h"

Http::Http(const char* buffer) {
    httpMessage = std::string(buffer);
    std::stringstream ss(httpMessage);
    ss >> method >> path >> protocol;

    // 读取请求头部
    size_t pos = httpMessage.find("Connection: ");
    if (pos != std::string::npos) {
        // 找到了Connection字段，现在找到该字段值的结束位置
        size_t end_pos = httpMessage.find("\r\n", pos);
        if (end_pos != std::string::npos) {
            // 返回Connection字段值
            connection =  httpMessage.substr(pos + 12, end_pos - (pos + 12));
        }
    }

    std::cout << connection << std::endl;
    if (this->path == "/" || this->path == "/.") {
        pf = "./";
    } else {
        pf = this->path.substr(1);
    }
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


int Http::response_http(struct bufferevent *bev) {
    if (this->method == "GET") {
        // get method ...
//        char decoded_path[this->path.size() + 1];
//        strcpy(decoded_path, path.c_str());
//        strdecode(decoded_path, decoded_path);
//        char *pf = &decoded_path[1];
//        if (this->path == "/" || this->path == "/.") {
//            pf = "./";
//        } else {
//            pf = this->path.substr(1);
//        }

//        printf("***** http Request Resource Path =  %s, pf = %s\n", decoded_path, pf);
        std::cout << "***** http Request Resource Path = " << this->path << " pf = " << this->pf << std::endl;
        struct stat sb;
        if (stat(this->pf.c_str(), &sb) < 0) {
            perror("open file err:");
            send_error(bev);
            return -1; // 返回-1表示出错
        }

        if (S_ISDIR(sb.st_mode)) {
            // 处理目录
            send_header(bev, 200, "OK", get_file_type(".html"), -1);
            send_dir(bev, pf.c_str());
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
     *以下是依据传递进来的文件名，使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
*/
const char* Http::get_file_type(const std::string& name) {
    size_t dot_pos = name.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "text/plain; charset=utf-8";
    }

    std::string extension = name.substr(dot_pos);
    if (extension == ".html" || extension == ".htm") {
        return "text/html; charset=utf-8";
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    } else if (extension == ".gif") {
        return "image/gif";
    } else if (extension == ".png") {
        return "image/png";
    } else if (extension == ".css") {
        return "text/css";
    } else if (extension == ".au") {
        return "audio/basic";
    } else if (extension == ".wav") {
        return "audio/wav";
    } else if (extension == ".avi") {
        return "video/x-msvideo";
    } else if (extension == ".mov" || extension == ".qt") {
        return "video/quicktime";
    } else if (extension == ".mpeg" || extension == ".mpe") {
        return "video/mpeg";
    } else if (extension == ".vrml" || extension == ".wrl") {
        return "model/vrml";
    } else if (extension == ".midi" || extension == ".mid") {
        return "audio/midi";
    } else if (extension == ".mp3") {
        return "audio/mpeg";
    } else if (extension == ".ogg") {
        return "application/ogg";
    } else if (extension == ".pac") {
        return "application/x-ns-proxy-autoconfig";
    }

    return "text/plain; charset=utf-8";
}


int Http::send_file_to_http(const std::string& filename, struct bufferevent *bev)
{
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open file err:");
        return -1;
    }

    char buf[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0)
    {
        bufferevent_write(bev, buf, bytes_read);
    }

    if (bytes_read < 0) {
        perror("read file err:");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int Http::send_header(struct bufferevent *bev, int no, const char* desp, const char *type, long len)
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

    // Connection: close/keep-alive
    if (this->connection == "keep-alive") {
        bufferevent_write(bev, "Connection: keep-alive\r\n", strlen("Connection: keep-alive\r\n"));
    } else {
        bufferevent_write(bev, _HTTP_CLOSE_, strlen(_HTTP_CLOSE_));
    }
    //send \r\n
    bufferevent_write(bev, "\r\n", 2);

    return 0;
}

int Http::send_error(struct bufferevent *bev)
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


int Http::send_dir(struct bufferevent *bev,const char *dirname)
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

/*
 * 这里的内容是处理%20之类的东西！是"解码"过程。
 * %20 URL编码中的‘ ’(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * 相关知识html中的‘ ’(space)是&nbsp
 */
void Http::strdecode(char *to, char *from)
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


// "编码"，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
// strencode(encoded_name, sizeof(encoded_name), name);
void Http::strencode(char* to, size_t tosize, const char* from)
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

//16进制数转化为10进制, return 0不会出现
int Http::hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

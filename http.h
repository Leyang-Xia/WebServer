#ifndef _HTTP_
#define _HTTP_

#include "headfile.h"

class Http {
public:
    std::string httpMessage;
    std::string method;
    std::string path;
    std::string protocol;
    std::string pf;

    explicit Http(const char* buffer);
    Http(Http&& other) noexcept;
    int response_http(struct bufferevent *bev);
    static int send_header(struct bufferevent *bev, int no, const char* desp, const char *type, long len);
    static int send_dir(struct bufferevent *bev,const char *dirname);
    static int send_error(struct bufferevent *bev);
    static int send_file_to_http(const std::string& filename, struct bufferevent *bev);
    static const char *get_file_type(const std::string& filename);
    static void strdecode(char *to, char *from);
    static void strencode(char* to, size_t tosize, const char* from);
    static int hexit(char c);
    ~Http() = default;
};

#endif
#ifndef _HEADFILE_
#define _HEADFILE_

#include <sstream>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <dirent.h>
#include <ctime>
#include <csignal>
#include <cctype>
#include <cerrno>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>

#define _HTTP_CLOSE_ "Connection: close\r\n"
#endif
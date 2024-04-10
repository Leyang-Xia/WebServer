#include "libeventHttp.h"


int main() {
    const char *targetDir = "/home/leyang/WebServer/resource";
    if(chdir(targetDir) < 0) {
        std::cout << "dir is not exists: " << targetDir << std::endl;
        perror("chdir error:");
        return -1;
    }

    run_http_server(8080);

    return 0;
}


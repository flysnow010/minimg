#ifndef HTTP_FILE_SERVER_H
#define HTTP_FILE_SERVER_H
#include <string>

struct HttpFileServer
{
    static void IgnoreSig();

    HttpFileServer(std::string const& docroot, std::string const& port);
    HttpFileServer(std::string const& docroot, int port);
    ~HttpFileServer();

    HttpFileServer( HttpFileServer & ) = delete;
    void operator = ( HttpFileServer ) = delete;
    bool run(int threadSize = 8);
private:
    struct HttpFileServerHandle* handle_;
};
#endif

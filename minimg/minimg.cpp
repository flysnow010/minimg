#include <HttpFileServer/HttpFileServer.h>

#include <iostream>

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        std::cerr << "usage: imageserver imagePath port" << std::endl;
        return -1;
    }
    HttpFileServer::IgnoreSig();
    HttpFileServer httpFileServer(argv[1], argv[2]);
    httpFileServer.run();
    return 0;
}
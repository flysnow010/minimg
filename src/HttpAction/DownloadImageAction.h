#ifndef DOWNLOAD_IMAGE_ACTION_H
#define DOWNLOAD_IMAGE_ACTION_H
#include "HttpAction.h"
#include <Magick++.h> 

struct DownloadImageAction : public HttpAction
{
    DownloadImageAction(evhtp_request_t* req, 
            std::string const& fileName,
            std::string const& imageSize);
    ~DownloadImageAction();
     enum { BufSize = 1024 };
    
    bool execute();
    void term();
private:
    int readData(const char* &data, int size);
private:
    struct evbuffer * buffer;
    Magick::Blob blob;
    size_t offset = 0;
};
#endif
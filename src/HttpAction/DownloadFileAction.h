#ifndef DOWNLOAD_FILE_ACTION_H
#define DOWNLOAD_FILE_ACTION_H
#include "HttpAction.h"

struct DownloadFileAction :  public HttpAction
{
    DownloadFileAction(evhtp_request_t* req, 
            std::string const& fileName,
            long long offset);
    ~DownloadFileAction();
    
    enum { BufSize = 1024 };

    bool execute();
    void term();
private:
    std::string getContentRange(long long offset, 
            std::string const& fileSize);
    std::string getFileSize(std::string const& fileName);
private:
    int file_;
    char data[BufSize];
    struct evbuffer * buffer;
};

#endif
#ifndef UPLOAD_FILE_ACTION_H
#define UPLOAD_FILE_ACTION_H
#include "HttpAction.h"
#include "HttpMultiPart.h"

struct UploadFileAction :  public HttpAction
{
    UploadFileAction(evhtp_request_t* req, 
            std::string const& filePath);
    ~UploadFileAction();
    enum { BufSize = 1024 };

    void setMultiPart(HttpMultiPart::Ptr multiPart)
    {
        multiPart_ = multiPart;
        multiPart_->setFilePath(filePath_);
    }

    bool execute();
    void term();
private:
    std::string filePath_;
    char data[BufSize];
    struct evbuffer * buffer;
    HttpMultiPart::Ptr multiPart_;
};

#endif

#include "UploadFileAction.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

extern "C" evhtp_res http_read_callback(evhtp_request_t * req,
    struct evbuffer* buffer,void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->execute();
    return EVHTP_RES_OK;
}
extern "C" evhtp_res request_close_callback(evhtp_request_t * req, void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->term();
    return EVHTP_RES_OK;
}
UploadFileAction::UploadFileAction(evhtp_request_t* req, 
        std::string const& filePath)
: HttpAction(req)
, filePath_(filePath)
, buffer(evbuffer_new())
{
    evhtp_connection_set_hook(req->conn, evhtp_hook_on_read,
            (evhtp_hook)http_read_callback, this);
    evhtp_connection_set_hook(req->conn, evhtp_hook_on_request_fini,
                    (evhtp_hook)request_close_callback, this);
}

UploadFileAction::~UploadFileAction()
{
    evhtp_safe_free(buffer, evbuffer_free);
}

bool UploadFileAction::execute()
{
    evbuffer_add_buffer(buffer, request->buffer_in);
    size_t minSize = multiPart_->minSize();
    while(evbuffer_get_length(buffer) > minSize)
    {
        size_t size = BufSize;
        char* text = evbuffer_readln(buffer, &size, EVBUFFER_EOL_CRLF_STRICT);
        if(text)
        {
            if(!multiPart_->parseLine(text, size))
            {
                evhtp_send_reply(request, EVHTP_RES_SERVERR);
            }
            free(text);
            if(multiPart_->isEnd())
            {
                evhtp_send_reply(request, EVHTP_RES_OK);
                break;
            }
        }
        else
        {
            size_t size = evbuffer_remove(buffer, data, BufSize);
            if(!multiPart_->writeData(data, size))
            {
                evhtp_send_reply(request, EVHTP_RES_SERVERR);
                break;
            }
        }
    }
    return true;
}

void UploadFileAction::term() { setState(StateTerm); }
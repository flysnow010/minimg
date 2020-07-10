#include "DownloadFileAction.h"
#include <boost/filesystem.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

extern "C" evhtp_res download_file_write_callback(struct evhtp_connection * c, void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->execute();
    return EVHTP_RES_OK;
}
extern "C" evhtp_res download_file_close_callback(struct evhtp_connection * c, void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->term();
    return EVHTP_RES_OK;
}

DownloadFileAction::DownloadFileAction(evhtp_request_t* req, std::string const& fileName,
    long long  offset)
: HttpAction(req)
, buffer(evbuffer_new())
{
    std::string fileSize = getFileSize(fileName);
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type","application/misc", 0, 0));
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Length",fileSize.c_str(), 0, 1));

    file_ = open(fileName.c_str(), O_RDONLY);
    if(file_ >= 0)
    {
        if(offset >= 0)
        {
            lseek(file_, offset, SEEK_SET);
            std::string contentRange = getContentRange(offset, fileSize);
            evhtp_headers_add_header(req->headers_out,
                evhtp_header_new("Content-Range", contentRange.c_str(), 0, 1));
        }
        evhtp_connection_set_hook(req->conn, evhtp_hook_on_write,
                (evhtp_hook)download_file_write_callback, this);
        evhtp_connection_set_hook(req->conn, evhtp_hook_on_connection_fini,
                (evhtp_hook)download_file_close_callback, this);
        if(offset >= 0)
            evhtp_send_reply_start(req, EVHTP_RES_PARTIAL);
        else
            evhtp_send_reply_start(req, EVHTP_RES_OK);
     }
}

std::string DownloadFileAction::getContentRange(long long offset, std::string const& fileSize)
{
    char text[1024];
    snprintf(text, sizeof(text), "bytes %lld-/%s",offset, fileSize.c_str());
    return std::string(text);
}

std::string DownloadFileAction::getFileSize(std::string const& fileName)
{
    try{
        return std::to_string(boost::filesystem::file_size(fileName));
    }
    catch(...){
        return std::string(); 
    }
}

DownloadFileAction::~DownloadFileAction()
{
    evhtp_safe_free(buffer, evbuffer_free);
    if(file_ >= 0)
        close(file_);
}

bool DownloadFileAction::execute()
{
    size_t    size = read(file_, data, BufSize);
    if(size <= 0)
    {
        evhtp_connection_unset_hook(request->conn, evhtp_hook_on_write);
        evhtp_send_reply_chunk_end(request);
        setState(StateEnd);
        return false;
    }

    evbuffer_add(buffer, data, size);
    evhtp_send_reply_chunk(request, buffer);
    evbuffer_drain(buffer, size);
    if(size < BufSize)
    {
        evhtp_connection_unset_hook(request->conn, evhtp_hook_on_write);
        evhtp_send_reply_chunk_end(request);
        setState(StateEnd);
    }
    return true;
}

void DownloadFileAction::term() { setState(StateTerm); }
#include "DownloadImageAction.h"

extern "C" evhtp_res download_image_write_callback(struct evhtp_connection * c, void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->execute();
    return EVHTP_RES_OK;
}
extern "C" evhtp_res download_image_close_callback(struct evhtp_connection * c, void * arg)
{
    HttpAction* action = (HttpAction *)arg;
    action->term();
    return EVHTP_RES_OK;
}

DownloadImageAction::DownloadImageAction(evhtp_request_t* req, 
        std::string const& fileName,
        std::string const& imageSize)
: HttpAction(req)
, buffer(evbuffer_new())      
{
    Magick::Image image(fileName);
    image.resize(imageSize);
    image.magick("JPEG");
    image.write(&blob);
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type","application/misc", 0, 0));
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Length", std::to_string(blob.length()).c_str(), 0, 1));

    evhtp_connection_set_hook(req->conn, evhtp_hook_on_write,
            (evhtp_hook)download_image_write_callback, this);
    evhtp_connection_set_hook(req->conn, evhtp_hook_on_connection_fini,
            (evhtp_hook)download_image_close_callback, this);
    evhtp_send_reply_start(req, EVHTP_RES_OK);
}
      
DownloadImageAction::~DownloadImageAction()
{
    evhtp_safe_free(buffer, evbuffer_free);
}

bool DownloadImageAction::execute()
{
    const char *data = nullptr;
    int size = readData(data, BufSize);
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

int DownloadImageAction::readData(const char* &data, int size)
{
    int data_size = blob.length() - offset;
    if(data_size == 0)
        return 0;
    
    data = static_cast<const char *>(blob.data()) + offset;
    if(size <= data_size)
    {
        offset += size;
        return size;
    }
    else
    {
        offset += data_size;
        return data_size;
        
    }
}

void DownloadImageAction::term() { setState(StateTerm); }
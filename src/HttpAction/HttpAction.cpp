#include "HttpAction.h"

#include <iostream>

HttpAction::HttpAction(evhtp_request_t* req)
: request(req)
, state(StateStart)
{
}

void HttpAction::send_reply(evhtp_res code, struct evbuffer * buf)
{
    evhtp_send_reply_start(request, code);
    evhtp_send_reply_body(request,  buf);
    evhtp_send_reply_end(request);
}

void printRequestHeaders(evhtp_request_t *req)
{
    evhtp_headers_t *headers = req->headers_out;
    evhtp_header_t *header;
    for (header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        std::cerr <<header->key << ":" << header->val << std::endl;
    }
}


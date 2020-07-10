#ifndef HTTP_ACTION_H
#define HTTP_ACTION_H
#include <evhtp.h>

#include <memory>

struct HttpAction
{
    HttpAction(evhtp_request_t* req);
    virtual ~HttpAction() {}
    
    HttpAction(HttpAction const&) = delete;
    HttpAction& operator=(HttpAction const&) = delete;
    
    using Ptr = std::shared_ptr<HttpAction>;

    virtual bool execute() = 0;
    virtual void term() {}

    enum State { StateStart, StateEnd,  StateTerm };

    evhtp_request_t *request;

    void setState(State s) { state = s; }
    State getState() const { return state; }
protected:
    void send_reply(evhtp_res code, struct evbuffer * buf);
private:
    State volatile state;
};
#endif

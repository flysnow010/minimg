#ifndef ACTION_QUEUE_H
#define ACTION_QUEUE_H
#include "HttpAction.h"

#include <list>

struct ActionQueue : private std::list<HttpAction::Ptr>
{
    enum { ExpirationTime = 300000 };

    void put(HttpAction::Ptr const& action, bool isFront = false);

    bool execute(evhtp_request_t *request);
    void removeTerm();
    bool isEmpty() const;
};

#endif
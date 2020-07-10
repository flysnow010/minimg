#include "ActionQueue.h"

void ActionQueue::put(HttpAction::Ptr const& action, bool isFront)
{
    if(isFront)
        push_front(action);
    else
        push_back(action);
}

bool ActionQueue::execute(evhtp_request_t *request)
{
    for(iterator it = begin(); it != end();  ++it)
    {
        if( (*it)->request == request)
            return (*it)->execute();
     }
     return false;
}

bool ActionQueue::isEmpty() const { return empty(); }

void ActionQueue::removeTerm()
{
    for(auto it = begin(); it != end();  ++it)
    {
        if((*it)->getState() == HttpAction::StateTerm)
         {
            it = erase(it);
            --it;
         }
     }
}
#ifndef DIR_ACTION_H
#define DIR_ACTION_H
#include "HttpAction.h"

struct DirAction : public HttpAction
{
    DirAction(evhtp_request_t* req, 
            std::string const& root,
            std::string const& path);

    bool execute();
private:
    void listJson(const std::string& pathName);
    void listHtml(const std::string& pathName);
    std::string getContentType();
    std::string fullPath() const { return root_ + path_; }
private:
    std::string root_;
    std::string path_;
};
#endif
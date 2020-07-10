#include "HttpFileServer.h"
#include "HttpAction/HttpAction.h"
#include "HttpAction/DirAction.h"
#include "HttpAction/DownloadFileAction.h"
#include "HttpAction/DownloadImageAction.h"
#include "HttpAction/UploadFileAction.h"

#include "HttpAction/ActionQueue.h"

#include <event.h>
#include <evhtp.h>

#include <iostream>
#include <sstream>
#include <list>
#include <map>
#include <memory>

#include <signal.h>
#include <sys/stat.h>
#include <string.h>
namespace
{

int string2int(std::string const& text)
{
    int value = 0;
    std::istringstream is(text);
    is >> value;
    return value;
}

struct Uri
{
    Uri(evhtp_uri_t *uri)
    {
        const char* path  = uri->path->full;
        const char* query =  (const char *)uri->query_raw;
        
        if(uri->query)
        {
            evhtp_kv* q = nullptr;
            TAILQ_FOREACH(q, uri->query, next)
                keyValues[q->key] = q->val;
        }

        path_ = uriDecode(path);
        query_ = uriDecode(query);

        size_t size = path_.size();
        if(size > 0 && path_[size-1] == '/')
            path_.erase(size-1);
    }

    std::string path() const { return path_; }

    std::string action() const
    {
        std::size_t start = path_.find(pathStart);
        if(start == std::string::npos)
            return std::string();
        start += pathStart.size();
        std::size_t end = path_.find(pathStop, start);
        if(end == std::string::npos)
            end = path_.size();
        return path_.substr(start, end - start);
    }
    std::string query() const { return query_; }
    bool hasKey(std::string const& key)
    { 
        return keyValues.count(key) > 0 ? true : false;
    }
    std::string valueByKey(std::string const& key){ return keyValues.at(key); }
    std::string uriDecode(const char* uri)
    {
        if(!uri)
            return std::string();

        char decodedUri[1024] = { 0 };
        char* ptr = decodedUri;
        evhtp_unescape_string((unsigned char**)&ptr, (unsigned char*)uri, strlen(uri));
        return std::string(decodedUri);
    }
private:
    std::string path_;
    std::string query_;
    std::map<std::string, std::string> keyValues;
    static std::string pathStart;
    static std::string pathStop;
};
std::string Uri::pathStart("/");
std::string Uri::pathStop("/");
}

extern "C" void timer_callback(evutil_socket_t fd, short events, void *arg);
struct HttpThread
{
    HttpThread(struct event_base * b, std::string const& root)
    : base(b)
    , docroot(root)
    {
        event_assign(&timer, base, -1, EV_PERSIST, timer_callback, this);
        add_timer(5);
    }
    
    using Ptr = std::shared_ptr<HttpThread>;
    
    void on_handler(evhtp_request_t *req)
    {
        htp_method method = evhtp_request_get_method(req);
        if(method == htp_method_GET)
            on_download(req);
        else if(method == htp_method_POST)
            actionQueue.execute(req);
        else
            evhtp_send_reply(req,  EVHTP_RES_NOTIMPL);
    }
    bool on_upload(evhtp_request_t * req)
    {
        const char*  contentType = evhtp_header_find(req->headers_in, "Content-Type");
        if(!contentType )
        {
            on_handler(req);
            return false;
        }

        HttpMultiPart::Ptr multiPart(new HttpMultiPart(contentType));
        if(!multiPart->isMultiPart())
        {
            on_handler(req);
            return false;
        }

        Uri uri(req->uri);
        std::string filePath = getFilePath(uri.path()); //docroot + uri.path();
        UploadFileAction* action = new UploadFileAction(req, filePath);
        action->setMultiPart(multiPart);
        action->execute();
        actionQueue.put(HttpAction::Ptr(action));
        return true;
    }
    void on_download(evhtp_request_t *req)
    {
        if(!req->uri)
            return;

        Uri uri(req->uri);
        std::string fileName = getFilePath(uri.path()); //docroot + uri.path();
        if(isDir(fileName))
            DirAction(req, docroot, uri.path()).execute();
        else
        {
            if(uri.hasKey("size")){
                HttpAction::Ptr action(new DownloadImageAction(req, 
                    fileName, uri.valueByKey("size")));
                actionQueue.put(action);
            }
            else{
                long long offset = getFileOffset(req);
                HttpAction::Ptr action(new DownloadFileAction(req, fileName, offset));
                actionQueue.put(action);
            }
        }
    }
    void on_timer()
    {
        actionQueue.removeTerm();
        add_timer(5);
    }
private:
    void add_timer(int sec)
    {
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = sec;
        event_add(&timer, &tv);
    }
    std::string getFilePath(std::string const& path)
    {
        struct stat sb;
        std::string filePath = docroot + path;

        if(stat(filePath.c_str(), &sb) >= 0)
            return filePath;
        return filePath;
    }
    bool isDir(const std::string& fileName)
    {
        struct stat sb;
        if(stat(fileName.c_str(), &sb) >= 0 && S_ISDIR(sb.st_mode))
            return true;
        return false;
    }
    long long getFileOffset(evhtp_request_t *req)
    {
        const char* value = evhtp_header_find(req->headers_in, "Range");

        if(!value)
            return -1;

        long long offset = 0;
        std::string range(value);//bytes=1000-

        std::size_t start =  range.find("=");
        if(start == std::string::npos)
            return offset;
        start += 1;
        std::size_t end = range.find("-", start);
        if(end == std::string::npos)
            end = range.size();
        std::istringstream is(range.substr(start, end - start));
        is >> offset;
        return offset;
    }
private:
    struct event_base * base;
    std::string docroot;
    struct event timer;
    ActionQueue actionQueue;
};
extern "C"  void timer_callback(evutil_socket_t fd, short events, void *arg)
{
    HttpThread *thread  = (HttpThread *)arg;
    thread->on_timer();
}

extern "C" evhtp_res http_connect_callback(evhtp_connection_t* conn, void *arg);
extern "C" void http_post_callback(evhtp_request_t * req, void * arg);
extern "C" void http_callback(evhtp_request_t * req, void * arg);
extern "C" void init_http_thread(evhtp_t *htp, evthr_t * thread, void *arg);
struct HttpFileServerHandle
{
    HttpFileServerHandle(std::string const& root, int p)
    : docroot(root)
    , port(p)
    , base(0)
    , http(0)
    {}

    ~HttpFileServerHandle()
    {
        evhtp_free(http);
        event_base_free(base);
    }

    bool run(int thread_size)
    {
        base = event_base_new();
        http    = evhtp_new(base, 0);
        evhtp_enable_flag(http,  EVHTP_FLAG_ENABLE_ALL);
        evhtp_set_gencb(http, http_callback,  this);
        evhtp_set_post_accept_cb(http, http_connect_callback, this);
        evhtp_use_threads_wexit(http, init_http_thread, 0, thread_size, this);
        evhtp_bind_socket(http, "0.0.0.0", port, 128);
        event_base_loop(base, 0);
        return true;
    }
    void addHttpThread(HttpThread * thread)
    {
        httpThreads.push_back(HttpThread::Ptr(thread));
    }

    std::string docroot;
    int port;
    struct event_base * base;
    struct evhtp      * http;
    std::list<HttpThread::Ptr> httpThreads;
};

extern "C" evhtp_res http_hears_callback(evhtp_request_t * req, evhtp_headers_t* hearders,
    void *arg)
{
    htp_method method = evhtp_request_get_method(req);
    if(method == htp_method_POST)
    {
        evhtp_connection_t* connect = evhtp_request_get_connection(req);
        HttpThread* httpThread = (HttpThread*)evthr_get_aux(connect->thread);
        if(httpThread->on_upload(req))
            return EVHTP_RES_OK;
        return EVHTP_RES_NOTIMPL;
    }
    if(method != htp_method_GET)
        return EVHTP_RES_NOTIMPL;
    return EVHTP_RES_OK;
}
extern "C" evhtp_res http_connect_callback(evhtp_connection_t* conn, void *arg)
{
    evhtp_connection_set_hook(conn, evhtp_hook_on_headers,
        (evhtp_hook)http_hears_callback, arg);
    return EVHTP_RES_OK;
}

extern "C" void http_callback(evhtp_request_t * req, void * arg)
{
    evhtp_connection_t* connect = evhtp_request_get_connection(req);
    HttpThread* httpThread = (HttpThread*)evthr_get_aux(connect->thread);
    httpThread->on_handler(req);
}

extern "C" void init_http_thread(evhtp_t *htp, evthr_t * thread, void *arg)
{
    HttpFileServerHandle *handle = (HttpFileServerHandle *)arg;
    struct event_base * base = evthr_get_base(thread);
    HttpThread* httpThread = new HttpThread(base, handle->docroot);
    handle->addHttpThread(httpThread);
    evthr_set_aux(thread, httpThread);
}

void HttpFileServer::IgnoreSig()
{
#ifndef WIN32
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signal_mask, 0);
#endif
}

HttpFileServer::HttpFileServer(std::string const& docroot,
        std::string const& port)
    : handle_(new HttpFileServerHandle(docroot, string2int(port)))
{
}

HttpFileServer::HttpFileServer(std::string const& docroot, int port)
    : handle_(new HttpFileServerHandle(docroot, port))
{
}

HttpFileServer::~HttpFileServer() { delete handle_; }

bool HttpFileServer::run(int threadSize)
{
    return handle_->run(threadSize);
}

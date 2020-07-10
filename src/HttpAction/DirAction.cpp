#include "DirAction.h"

#include<boost/filesystem.hpp>
#include <list> 
#include <ctime>

using namespace boost;
namespace
{
inline std::string formatDateTime(std::time_t time)
{
    char tmp[64];
    std::strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&time));
    return std::string(tmp);
}
}
DirAction::DirAction(evhtp_request_t* req, 
        std::string const& root, 
        std::string const& path)
: HttpAction(req)
, root_(root)
, path_(path)
{
}

std::string DirAction::getContentType()
{
    const char* val = evhtp_header_find(request->headers_in, "Content-Type");
    if(val)
        return std::string(val);
    return std::string();
}

bool DirAction::execute()
{
    listHtml(fullPath());
    return true;
}

void DirAction::listHtml(const std::string& pathName)
{
    filesystem::path dir(path_);
    evhtp_headers_add_header(request->headers_out,
            evhtp_header_new("Content-Type", "text/html", 0, 0));

        evbuffer_add_printf(request->buffer_out,
                "<!DOCTYPE html>\n"
                "<html>\n <head>\n"
                "  <meta charset='utf-8'>\n"
                "  <title>Minimg</title>\n"
                "  <style>"
                "   table { width: 100% ;background: #EEEEEE  }"
                "   thead th { padding-left: 3px; text-align: left; background: grey; color: white}"
                "   tbody tr { background: #DDDDDD; }"           
                "   tbody th { text-align: center; background: lightgrey; color: grey}"
                "   tbody td { padding-left: 3px; }"
                "   #id { text-align: center;}"
                "   a{text-decoration: none;}"
                "   a:visited{color:blue;}"
                "  </style>"
                " </head>\n"
                " <body>\n"
                " <h1><a href=\"%s\">%s</a></h1>\n"
                , dir.parent_path().generic_string().c_str()
                , dir.filename().generic_string().c_str());
        evbuffer_add_printf(request->buffer_out, R"(
                <table>
                <thead>
                <tr>
                <th id="id" style="width: 32px">#</th>
                <th>文件名</th>
                <th style="width: 150px">大小</th>
                <th style="width: 200px">修改日期</th>
                </tr>
                </thead>            
            )");
        evbuffer_add_printf(request->buffer_out, "</thead>");
        evbuffer_add_printf(request->buffer_out, "<tbody>");
        filesystem::path root(pathName);
        filesystem::directory_iterator end;
       
        std::list<filesystem::path> files;
        std::list<filesystem::path> dirs;
        for(filesystem::directory_iterator it(root); it != end; ++it)
        {
            auto path = it->path();
            if(filesystem::is_regular(path))
                files.push_back(path);
            else
                dirs.push_back(path);
        }
        int id = 1;
        for(auto const& path : dirs)
        {
            evbuffer_add_printf(request->buffer_out, "<tr>");
            evbuffer_add_printf(request->buffer_out, "<th>%d</th>", id++);
            std::string fileName = path.filename().generic_string();
            std::string urlName = path_ + "/" + fileName;
            evbuffer_add_printf(request->buffer_out,
                "    <td><a href=\"%s\">%s</a></td><td></td><td>%s</td>\n",
                urlName.c_str(), fileName.c_str(),
                formatDateTime(filesystem::last_write_time(path)).c_str());
        }
        for(auto const& path : files)
        {
            evbuffer_add_printf(request->buffer_out, "<tr>");
            evbuffer_add_printf(request->buffer_out, "<th>%d</th>", id++);
            std::string fileName = path.filename().generic_string();
            std::string urlName = path_ + "/" + fileName;
            evbuffer_add_printf(request->buffer_out,
                "    <td><a href=\"%s\">%s</a></td><td>%ld</td><td>%s</td>\n",
                urlName.c_str(), fileName.c_str(), 
                filesystem::file_size(path),
                formatDateTime(filesystem::last_write_time(path)).c_str());
        }
        evbuffer_add_printf(request->buffer_out, "</tbody>");
        evbuffer_add_printf(request->buffer_out, "</table></body></html>\n");
        send_reply(EVHTP_RES_OK, request->buffer_out);
}
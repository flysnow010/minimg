#include "HttpMultiPart.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <vector>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

namespace
{
    std::string getContentType(std::string const& line)
    {
        std::vector<std::string> items;
        boost::split(items, line, boost::is_any_of(":"));
        if(items.size() > 1){
            boost::trim(items[1]);
            return items[1];
        }
        return std::string();
    }
    std::string parseValueOfKey(std::string const& kv, std::string const& key)
    {
        auto p = kv.find(key);
        if(p == std::string::npos)
            return std::string();
        auto value =  kv.substr(p + key.size());
        p = value.find('=');
        if(p == std::string::npos)
            return std::string();
        value = value.substr(p + 1);

        if(value.empty())
            return std::string();
        if(value[0] == '"')
            value.erase(0, 1);

        if(value.empty())
            return std::string();
        if(value[value.size() - 1] == '"')
            value.erase(value.size() - 1, 1);
        return value;
    }
}

HttpMultiPart::~HttpMultiPart()
{
    if(file >= 0)
    {
        close(file);
        file = -1;
    }
}

bool HttpMultiPart::isMultiPart()
{
    auto found = contentType.find("multipart/form-data");
    if (found == std::string::npos)
        return false;
    std::vector<std::string> items;
    boost::split(items, contentType, boost::is_any_of(":"));
    if(items.size() < 2)
        return false;

    boundary  = std::string("--") + parseValueOfKey(items[1], "boundary");
    boundaryEnd = boundary + std::string("--");
    return true;
}

bool HttpMultiPart::parseLine(const char* text, ssize_t size)
{
    std::string line(text);

    if(line == boundary)// step: 1
    {
        multiPartState = MultiPartState::Boundary;
        if(file >= 0)
        {
            close(file);
            file = -1;
        }
    }
    else if(line.find("Content-Type") != std::string::npos) //step: 2
    {
        dataType = getContentType(line) ;//file
    }
    else if(line.find("Content-Disposition") != std::string::npos)//step: 3
    {
        if(parseContentDisposition(line))
            multiPartState = MultiPartState::Disposition;
    }
    else if(line.empty()) //step: 4
    {
        if(multiPartState == MultiPartState::Disposition)
        {
            multiPartState = MultiPartState::Data;
            std::string filename = filePath + "/" + fileName;//??
            boost::filesystem::create_directories(filePath);
            file = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
            firstWrite = true;
            return true;
        }
    }
    else if(line == boundaryEnd)
    {
        if(file >= 0)
        {
            if(!close(file))
            {
                file = -1;
            }
        }
        isEnd_ = true;
        return true;
    }
    if(file >= 0)
    {
        if(firstWrite)
        {
            if(write(file, text, size) != size)
                return false;
            firstWrite = false;
        }
        else
        {
            if(write(file,  "\r\n", 2) != 2)
                return false;
            if(write(file, text, size) != size)
                return false;
        }
    }
    return true;
}

bool HttpMultiPart::writeData(char* data, ssize_t size)
{
    if(!firstWrite)
    {
        if(write(file,  "\r\n", 2) != 2)
            return false;
        firstWrite = true;
    }
    if(write(file, data, size) != size)
        return false;
    return true;
}

//Content-Disposition: form-data; name="files[]";filename="message.json"
bool  HttpMultiPart::parseContentDisposition(std::string const& line)
{
    std::vector<std::string> items;
    boost::split(items, line, boost::is_any_of(":"));
    if(items.size() < 2)
        return false;

    std::string value = items[1];
    auto p = value.find("form-data");
    if(p == std::string::npos)
        return false;

    boost::split(items, value, boost::is_any_of(";"));
    if(items.size() < 3)
        return false;
    name = parseValueOfKey(items[1], "name");
    fileName = parseValueOfKey(items[2], "filename");
    return true;
}

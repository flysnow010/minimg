#ifndef HTTP_MULTI_PART_H
#define HTTP_MULTI_PART_H

#include <memory>
#include <string>

enum class MultiPartState
{
    None,
    Boundary,
    Disposition,
    Data,
    BoundaryEnd
};

struct HttpMultiPart
{
    HttpMultiPart(std::string const& contenttype)
        : contentType(contenttype)
        , multiPartState(MultiPartState::None)
        , file(-1)
        , firstWrite(false)
        , isEnd_(false)
    {}

    ~HttpMultiPart();
    
    HttpMultiPart(HttpMultiPart const&) = delete;
    HttpMultiPart& operator=(HttpMultiPart const&) = delete;
    
    using Ptr = std::shared_ptr<HttpMultiPart>;

    bool isMultiPart();
    void setFilePath(std::string const& path) { filePath = path;  }

    bool isEnd() const { return isEnd_; }
    size_t minSize() const {  return boundaryEnd.size(); }

    bool parseLine(const char* text, ssize_t size);
    bool writeData(char* data, ssize_t size);
private:
    bool  parseContentDisposition(std::string const& line);
private:
    std::string contentType;
    std::string boundary;
    std::string boundaryEnd;

    MultiPartState multiPartState;
    std::string dataType;
    std::string filePath;
    std::string name;
    std::string fileName;
    int file;
    bool firstWrite;
    bool isEnd_;
};
#endif

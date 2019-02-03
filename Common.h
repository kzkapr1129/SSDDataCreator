#pragma once

#include <vector>
#include <string>

struct Config {
    std::string images;
    std::string ext;
    std::vector<std::string> labels;
    std::string outImages;
    std::string csvFilename;
    std::string xmlFolderName;
    std::string xmlDatabaseName;
    std::string xmlAnnotationName;
    std::string xmlOwnerName;
};

class Dir {
public:
    Dir(const std::string& dname, const std::string& ext);
    std::string next();

private:
    std::vector<std::string> mFilelist;
    std::vector<std::string>::iterator mSeek;
};
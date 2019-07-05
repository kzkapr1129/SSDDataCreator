#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

struct Config {
    std::string images;
    std::string ext;
    std::string outExt;
    std::vector<std::string> labels;
    std::string outImages;
    std::string outAnnotations;
    std::string csvFilename;
    std::string outPrefix;
    std::string xmlFolderName;
    std::string xmlDatabaseName;
    std::string xmlAnnotationName;
    std::string xmlOwnerName;
    bool enableRota;
};

struct LabelData {
    int classId;
    std::string filename;
    cv::Scalar color;
    std::string text;

    cv::Point min;
    cv::Point max;
};

class Dir {
public:
    Dir(const std::string& dname, const std::string& ext);
    std::string next();
    void prev();

private:
    std::vector<std::string> mFilelist;
    std::vector<std::string>::iterator mSeek;
};

std::string format(const char* format, ...);
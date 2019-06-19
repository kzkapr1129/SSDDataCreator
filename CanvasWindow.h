#pragma once

#include "Common.h"
#include <string>
#include <opencv2/opencv.hpp>

typedef void (*OnSaveFunc)(void* userdata,
    const std::string& filename,
    const cv::Mat& img,
    const std::vector<LabelData>& labels);

class CanvasWindow {
public:
    CanvasWindow();

    void setCallback(OnSaveFunc saveFunc, void* userdata);
    void setImage(const std::string& filename);
    void setClass(int classId, const cv::Scalar& color, const std::string& text);

    void clear();
    bool isNeededImage();

private:
    static void onMouseEvent(int eventType, int x, int y, int flags, void* userdata);
    void redraw();
    void drawLabel(const LabelData& label);
    void save();

    bool mIsNeeded;

    OnSaveFunc mSaveFunc;
    void* mUserData;
    
    std::string mFilename;
    cv::Mat mSrc;
    cv::Mat mWork;

    LabelData mFocus;
    std::vector<LabelData> mLabels;

    int mClassId;
    cv::Scalar mColor;
    std::string mText;
};
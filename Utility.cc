#include "Utility.h"
#include <opencv2/opencv.hpp>

#define RADIAN(angle) ((angle) * M_PI / 180)

void rotate_point(const cv::Point& src,
        cv::Point& dst, const cv::Point& center, float angle) {
    float rad = RADIAN(angle);
    dst = src - center; 
    int x = dst.x * cos(rad) - dst.y * sin(rad) + center.x;
    int y = dst.x * sin(rad) + dst.y * cos(rad) + center.y;
    dst.x = x;
    dst.y = y;
}

void rotate_img(const cv::Mat& src, cv::Mat& dst, int angle) {
    if(angle == 270 || angle == -90) {
        cv::transpose(src, dst);
        cv::flip(dst, dst, 0);
    } else if(angle == 180 || angle == -180) {
        cv::flip(src, dst, -1);
    } else if(angle == 90 || angle == -270) {
        cv::transpose(src, dst);
        cv::flip(dst, dst, 1);
    } else if(angle == 360 || angle == 0 || angle == -360){
        if(src.data != dst.data){
            src.copyTo(dst);
        }
    }
}

void rotate_data(const cv::Mat& frame,
        const std::vector<LabelData>& label,
        cv::Mat& newFrame,
        std::vector<LabelData>& newLabel,
        int angle) {

    // 画像回転
    rotate_img(frame, newFrame, angle);

    // ラベル回転
    newLabel.clear();
    auto it = label.begin();
    auto end = label.end();
    for (; it != end; it++) {
        LabelData data = *it;

        cv::Point center(frame.cols/2, frame.rows/2);

        cv::Point min, max;
        rotate_point(it->min, min, center, angle);
        rotate_point(it->max, max, center, angle);
        data.min.x = std::min(min.x, max.x);
        data.min.y = std::min(min.y, max.y);
        data.max.x = std::max(min.x, max.x);
        data.max.y = std::max(min.y, max.y);

        newLabel.push_back(data);
    }
}
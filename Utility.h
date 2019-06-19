#pragma once

#include "Common.h"

void rotate_point(const cv::Point& src,
        cv::Point& dst, const cv::Point& center, float angle);
void rotate_img(const cv::Mat& src, cv::Mat& dst, int angle);
void rotate_data(const cv::Mat& frame,
        const std::vector<LabelData>& label,
        cv::Mat& newFrame,
        std::vector<LabelData>& newLabel,
        int angle);
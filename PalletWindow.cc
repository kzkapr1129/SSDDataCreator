//
//  PalletWindow.cpp
//  SegnetTestDataCreator
//
//  Created by 中山一輝 on 2018/11/24.
//  Copyright © 2018年 中山一輝. All rights reserved.
//

#include "PalletWindow.h"
#include <opencv2/opencv.hpp>

static const int PALLTE_WINDOW_WIDTH = 500;
static const int PALLTE_WINDOW_HEIGHT = PALLTE_WINDOW_WIDTH * 3/4;
static const int TILE_W = PALLTE_WINDOW_WIDTH / 4;
static const int TILE_H = PALLTE_WINDOW_HEIGHT / 3;
static const int NUMBER_MERGIN_Y = 30;
static const int FOCUSED_RECT_MERGIN = 3;
static const char* PALLET_WINDOW_NAME = "class pallet";

struct Class {
    cv::Rect rect;
    cv::Scalar color;
    int classId;
    Class(const cv::Rect& r, const cv::Scalar& c, int id)
    : rect(r), color(c), classId(id) {}
};

static const Class CLASSES[] = {
    Class(cv::Rect(       0,        0, TILE_W, TILE_H), cv::Scalar(  0,  0,  0), 0),
    Class(cv::Rect(  TILE_W,        0, TILE_W, TILE_H), cv::Scalar(128,  0,  0), 1),
    Class(cv::Rect(TILE_W*2,        0, TILE_W, TILE_H), cv::Scalar(255,  0,  0), 2),
    Class(cv::Rect(TILE_W*3,        0, TILE_W, TILE_H), cv::Scalar(  0,128,  0), 3),
    Class(cv::Rect(       0,   TILE_H, TILE_W, TILE_H), cv::Scalar(  0,255,  0), 4),
    Class(cv::Rect(  TILE_W,   TILE_H, TILE_W, TILE_H), cv::Scalar(  0,  0,128), 5),
    Class(cv::Rect(TILE_W*2,   TILE_H, TILE_W, TILE_H), cv::Scalar(  0,  0,200), 6),
    Class(cv::Rect(TILE_W*3,   TILE_H, TILE_W, TILE_H), cv::Scalar(128,128,  0), 7),
    Class(cv::Rect(       0, TILE_H*2, TILE_W, TILE_H), cv::Scalar(255,255,  0), 8),
    Class(cv::Rect(  TILE_W, TILE_H*2, TILE_W, TILE_H), cv::Scalar(  0,128,128), 9),
    Class(cv::Rect(TILE_W*2, TILE_H*2, TILE_W, TILE_H), cv::Scalar(  0,255,255), 10),
    Class(cv::Rect(TILE_W*3, TILE_H*2, TILE_W, TILE_H), cv::Scalar(128,  0,128), 11)
};
static const size_t NUM_CLASSES = sizeof(CLASSES) / sizeof(Class);

PalletWindow::PalletWindow(const std::vector<std::string>& labels)
    : mFocusedClass(0), mCallbackFunc(NULL), mUserdata(NULL), mLabels(labels) {

    draw();    
    cv::setMouseCallback(PALLET_WINDOW_NAME, onPalletChanged, this);
}

void PalletWindow::setCallback(OnClassChangedFunc func, void* userdata) {
    mCallbackFunc = func;
    mUserdata = userdata;
}

const cv::Scalar& PalletWindow::getFocusedClassColor() const {
    return CLASSES[mFocusedClass].color;
}

int PalletWindow::getFocusedClassId() const {
    return CLASSES[mFocusedClass].classId;
}

void PalletWindow::draw() {
    cv::Mat img = cv::Mat::zeros(PALLTE_WINDOW_HEIGHT, PALLTE_WINDOW_WIDTH, CV_8UC3);
    
    for (int i = 0; i < NUM_CLASSES; i++) {
        cv::Mat roi = img(CLASSES[i].rect);
        roi.setTo(CLASSES[i].color);
        
        if (mFocusedClass == i) {
            cv::Rect focusRect(
                    cv::Point(FOCUSED_RECT_MERGIN, FOCUSED_RECT_MERGIN),
                    cv::Point(roi.cols-FOCUSED_RECT_MERGIN, roi.rows-FOCUSED_RECT_MERGIN));
            cv::rectangle(roi, focusRect.tl(), focusRect.br(), cv::Scalar(128,128, 255), 3);
        }
        
        cv::putText(roi, mLabels[i].c_str(), cv::Point(0, NUMBER_MERGIN_Y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255));
    }
    
    cv::imshow(PALLET_WINDOW_NAME, img);
}

void PalletWindow::onPalletChanged(int eventType, int x, int y, int flags, void* userdata) {
    PalletWindow* self = (PalletWindow*)userdata;

    cv::Point touchPos(x, y);
    if (eventType == cv::EVENT_LBUTTONDOWN) {
        int tochedClassIndex = -1;
        for (int i = 0; i < NUM_CLASSES; i++) {
            if (CLASSES[i].rect.contains(touchPos)) {
                tochedClassIndex = i;
                break;
            }
        }
        
        if (tochedClassIndex >= 0 && tochedClassIndex < NUM_CLASSES) {
            self->mFocusedClass = tochedClassIndex;
            self->draw();
            if (self->mCallbackFunc) {
                self->mCallbackFunc(self->mUserdata);
            }
        }
    }
}

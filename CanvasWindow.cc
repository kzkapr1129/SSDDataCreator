#include "CanvasWindow.h"

#define DEBUG_SAVING

static const int CIRCLE_RADIUS = 3;
static const int PADDING = 20;
static const char* CANVAS_WINDOW_NAME = "class canvas";

CanvasWindow::CanvasWindow() : mIsNeeded(true) {
    cv::namedWindow(CANVAS_WINDOW_NAME);
    cv::setMouseCallback(CANVAS_WINDOW_NAME, onMouseEvent, this);
}

void CanvasWindow::setCallback(OnSaveFunc saveFunc, void* userdata) {
    mSaveFunc = saveFunc;
    mUserData = userdata;
}

void CanvasWindow::setImage(const std::string& filename) {
    cv::Mat img = cv::imread(filename);
    if (img.data == NULL) {
        mIsNeeded = true;
        return;
    }

    mFilename = filename;
    mSrc = img;

    // キャンパスの初期化
    clear();

    // 再描画
    redraw();

    mIsNeeded = false;
}

void CanvasWindow::setClass(int classId, const cv::Scalar& color, const std::string& text) {
    mClassId = classId;
    mColor = color;
    mText = text;

    mFocus.min = cv::Point(-1, -1);
    mFocus.max = cv::Point(-1, -1);
    mFocus.classId = mClassId;
    mFocus.color = mColor;
    mFocus.text = mText;

    // 再描画
    redraw();
}

void CanvasWindow::clear() {
    // フォーカスの初期化
    mFocus.min = cv::Point(-1, -1);
    mFocus.max = cv::Point(-1, -1);
    mFocus.classId = mClassId;
    mFocus.color = mColor;
    mFocus.text = mText;
    mLabels.clear();
}

bool CanvasWindow::isNeededImage() {
    return mIsNeeded;
}

void CanvasWindow::onMouseEvent(int eventType, int x, int y, int flags, void* userdata) {
    CanvasWindow* self = (CanvasWindow*)userdata;
    if (self->isNeededImage()) {
        // 画像を持っていないときは何もせずに終了する
        return;
    }

    bool redraw = false;
    if (eventType == cv::EVENT_LBUTTONDOWN) {
        if (self->mFocus.min.x < 0 || self->mFocus.min.y < 0) {
            self->mFocus.min.x = x;
            self->mFocus.min.y = y;
        } else if (self->mFocus.max.x < 0 || self->mFocus.max.y < 0) {
            self->mFocus.max.x = x;
            self->mFocus.max.y = y;
            self->mLabels.push_back(self->mFocus);
        } else {
            self->mFocus.min.x = x;
            self->mFocus.min.y = y;
            self->mFocus.max = cv::Point(-1, -1);
        }
        redraw = true;
    } else if (eventType == cv::EVENT_RBUTTONDOWN) {
        // 今回と前回の結果をクリア
        self->mFocus.min = cv::Point(-1, -1);
        self->mFocus.max = cv::Point(-1, -1);
        if (0 < self->mLabels.size()) {
            self->mLabels.erase(self->mLabels.begin() + (self->mLabels.size() - 1));
        }
        redraw = true;
    } else if (eventType == cv::EVENT_MBUTTONUP) {
        self->save();
    }

    if (redraw) {
        self->redraw();
    }
}

void CanvasWindow::redraw() {
    if (mSrc.data == NULL) return;

    // 画面のクリア
    mWork = cv::Mat::zeros(mSrc.rows + PADDING * 2, mSrc.cols + PADDING * 2, CV_8UC3);
    mWork = cv::Scalar(255, 255, 255);
    cv::Rect rect(PADDING, PADDING, mSrc.cols, mSrc.rows);
    cv::Mat roi = mWork(rect);
    mSrc.copyTo(roi);

    drawLabel(mFocus);

    auto it = mLabels.begin();
    auto end = mLabels.end();
    for (; it != end; it++) {
        drawLabel(*it);
    }

    // 画像の表示
    cv::imshow(CANVAS_WINDOW_NAME, mWork);
}

void CanvasWindow::drawLabel(const LabelData& label) const {
    int count = 0;

    // 始点を描画
    if (label.min.x >= 0 && label.min.y >= 0) {
        cv::circle(mWork, label.min, CIRCLE_RADIUS, label.color, -1);
        count++;
    }

    // 終点を描画
    if (label.max.x >= 0 && label.max.y >= 0) {
        cv::circle(mWork, label.max, CIRCLE_RADIUS, label.color, -1);
        count++;
    }

    // 枠の描画
    if (count == 2) {
        int x = label.min.x < label.max.x ? label.min.x : label.max.x;
        int y = label.min.y < label.max.y ? label.min.y : label.max.y;
        int width = label.min.x < label.max.x ? label.max.x - label.min.x : label.min.x - label.max.x;
        int height = label.min.y < label.max.y ? label.max.y - label.min.y : label.min.y - label.max.y;
        cv::Rect rect(x, y, width, height);
        cv::rectangle(mWork, rect, label.color, 2);
    }
}

void CanvasWindow::save() {
    if (mSrc.data == NULL) {
        mIsNeeded = true;
        return;
    }

    // ラベルデータを保存に適した状態に変換する
    auto it = mLabels.begin();
    auto end = mLabels.end();
    for (; it != end; it++) {
        const LabelData& data = *it;

        // min, maxの座標に直す
        int minX = data.min.x < data.max.x ? data.min.x : data.max.x;
        int maxX = data.min.x < data.max.x ? data.max.x : data.min.x;
        int minY = data.min.y < data.max.y ? data.min.y : data.max.y;
        int maxY = data.min.y < data.max.y ? data.max.y : data.min.y;

        // パディングの削除
        minX = std::min(mSrc.cols, std::max(0, minX - PADDING));
        minY = std::min(mSrc.rows, std::max(0, minY - PADDING));
        maxX = std::min(mSrc.cols, std::max(0, maxX - PADDING));
        maxY = std::min(mSrc.rows, std::max(0, maxY - PADDING));

        it->min.x = minX;
        it->min.y = minY;
        it->max.x = maxX;
        it->max.y = maxY;

    }

#ifdef DEBUG_SAVING
    cv::Mat debugImg = mSrc.clone();
    it = mLabels.begin();
    end = mLabels.end();
    for (; it != end; it++) {
        const LabelData& data = *it;
        cv::Rect rect(data.min.x, data.min.y, data.max.x - data.min.x, data.max.y - data.min.y);
        cv::rectangle(debugImg, rect, it->color, 2);
    }
    cv::imwrite("debug.png", debugImg);
#endif

    mSaveFunc(mUserData, mFilename, mSrc, mLabels);
    clear();
    mIsNeeded = true;
}
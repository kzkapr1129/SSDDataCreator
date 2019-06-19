#include "AnnoData.h"
#include "Common.h"
#include "CanvasWindow.h"
#include "PalletWindow.h"
#include "Utility.h"
#include <opencv2/opencv.hpp>
#include <json-c/json.h>
#include <sys/stat.h>
#include <fstream>

static const int ACCESS_FLAG = S_IRGRP | S_IROTH | S_IRUSR | S_IRWXO | S_IRWXU | S_IWOTH;

struct GlobalData {
    Dir* dir;
    Config* config;
    PalletWindow* pallet;
    CanvasWindow* canvas;

    int index;

    GlobalData() : dir(NULL), config(NULL), pallet(NULL), canvas(NULL), index(0) {}
};

static void onClassChanged(void* userdata) {
    GlobalData* data = static_cast<GlobalData*>(userdata);

    const cv::Scalar& color = data->pallet->getFocusedClassColor();
    int classId = data->pallet->getFocusedClassId();
    const std::string& text = data->config->labels[classId];

    data->canvas->setClass(classId, color, text);
}

static void correctBrightness(cv::Mat& frame, float bright_scale) {
    if (bright_scale == 0.0) {
        return;
    }

	cv::Mat hsv;
	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

	std::vector<cv::Mat> channels;
	cv::split(hsv, channels);
	channels[2] *= bright_scale;
	cv::merge(channels, hsv);
	cv::cvtColor(hsv, frame, cv::COLOR_HSV2BGR);
}

static void saveDebug(const std::string& frameName, cv::Mat& frame, const std::vector<LabelData>& labels) {
    auto it = labels.begin();
    auto end = labels.end();
    for (; it != end; it++) {
        const LabelData& data = *it;

        cv::Rect rect(data.min, data.max);
        cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);
    }

    cv::imwrite(frameName, frame);
}

static void save(void* userdata,
        const std::string& filename,
        const cv::Mat& img,
        const std::vector<LabelData>& labels,
        float brightnessScale,
        const std::string& debugName) {

    cv::Mat bimg = img.clone();
    correctBrightness(bimg, brightnessScale);

    GlobalData* data = static_cast<GlobalData*>(userdata);

    time_t now = time(NULL);
    std::string baseFilename = format("%llu_%d", (uint64_t)now, data->index++);
    if (0 < data->config->outPrefix.length()) {
        baseFilename = data->config->outPrefix + baseFilename;
    }

    std::string imageFilename = data->config->outImages + baseFilename + data->config->outExt;
    std::string xmlFilename = data->config->outAnnotations + baseFilename + ".xml";
    std::string xml = AnnoData::genXml(*data->config, img.cols, img.rows, imageFilename, labels);

    // 出力フォルダに画像データを保存
    cv::imwrite(imageFilename.c_str(), bimg);

    // 出力フォルダにアノテーション(xml)を保存
    FILE* fp = fopen(xmlFilename.c_str(), "w");
    if (fp) {
        fwrite(xml.c_str(), sizeof(xml.c_str()[0]), xml.length(), fp);
        fclose(fp);
    }

    // csvの出力
    fp = fopen(data->config->csvFilename.c_str(), "a");
    if (fp) {
        fprintf(fp, "%s %s\n", imageFilename.c_str(), xmlFilename.c_str());
        fclose(fp);
    }

    cv::Mat debugFrame = img.clone();
    saveDebug(debugName, debugFrame, labels);
}

static void saveAndRota(void* userdata,
        const std::string& filename,
        const cv::Mat& img,
        const std::vector<LabelData>& labels) {

    const int ANGLES[] = {0, 90, 180, 270};
    const int NUM_ANGLES = sizeof(ANGLES) / sizeof(ANGLES[0]);

    for (int i = 0; i < NUM_ANGLES; i++) {
        cv::Mat rotatedFrame;
        std::vector<LabelData> rotatedLabels;
        rotate_data(img, labels, rotatedFrame, rotatedLabels, ANGLES[i]);

        save(userdata, filename, rotatedFrame, rotatedLabels, 1.1, std::to_string(ANGLES[i]) + "_11.png");
        save(userdata, filename, rotatedFrame, rotatedLabels, 0.0, std::to_string(ANGLES[i]) + "_00.png");
        save(userdata, filename, rotatedFrame, rotatedLabels, 0.9, std::to_string(ANGLES[i]) + "_09.png");
    }
}

static void onSave(void* userdata,
        const std::string& filename,
        const cv::Mat& img,
        const std::vector<LabelData>& labels) {

    // ソースの画像データを削除
    remove(filename.c_str());
    saveAndRota(userdata, filename, img, labels);
}

static int loadConfig(Config* config) {
    // 設定ファイルの読み込み
    json_object *pobj = json_object_from_file("./config.json");
    if (pobj == NULL) {
        fprintf(stderr, "config.jsonの読み込みに失敗しました\n");
        return -1;
    }

    json_object_object_foreach(pobj, pkey, pval) {
        if (json_object_is_type(pval, json_type_object)) {
            auto cobj = pval;
            json_object_object_foreach(cobj, ckey, cval) {
                if (json_object_is_type(cval, json_type_array)) {
                    if (!strcmp("input_settings", pkey)) {
                        if (!strcmp("labels", ckey)) {
                            for (int i = 0; i < json_object_array_length(cval); ++i) {
                                struct json_object *a = json_object_array_get_idx(cval, i);
                                const char* labelText = json_object_get_string(a);
                                config->labels.push_back(labelText);
                            }
                        }
                    }
                } else if (json_object_is_type(cval, json_type_string)) {
                    if (!strcmp("input_settings", pkey)) {
                        if (!strcmp("images", ckey)) {
                            config->images = json_object_get_string(cval);
                        } else if (!strcmp("target_ext", ckey)) {
                            config->ext = json_object_get_string(cval);
                        }
                    } else if (!strcmp("output_settings", pkey)) {
                        if (!strcmp("csv_filename", ckey)) {
                            config->csvFilename = json_object_get_string(cval);
                        } else if (!strcmp("out_prefix", ckey)) {
                            config->outPrefix = json_object_get_string(cval);
                        } else if (!strcmp("out_images", ckey)) {
                            config->outImages = json_object_get_string(cval);
                        } else if (!strcmp("xml_foldername", ckey)) {
                            config->xmlFolderName = json_object_get_string(cval);
                        } else if (!strcmp("xml_databasename", ckey)) {
                            config->xmlDatabaseName = json_object_get_string(cval);
                        } else if (!strcmp("xml_annotationname", ckey)) {
                            config->xmlAnnotationName = json_object_get_string(cval);
                        } else if (!strcmp("xml_ownername", ckey)) {
                            config->xmlOwnerName = json_object_get_string(cval);
                        } else if (!strcmp("out_img_ext", ckey)) {
                            config->outExt = json_object_get_string(cval);
                        } else if (!strcmp("out_annotations", ckey)) {
                            config->outAnnotations = json_object_get_string(cval);
                        }
                    }
                }
            }
        }
    }

    // 不正な設定ファイルのチェック
    if (config->labels.size() != 12) {
        fprintf(stderr, "labelsの数が足りません: %lu\n", config->labels.size());
        return -1;
    }

    // データの修正
    if (config->outImages.back() != '/') {
        config->outImages += '/';
    }

    // データの修正
    if (config->outAnnotations.back() != '/') {
        config->outAnnotations += '/';
    }

    return 0;
}

static bool copyFile(const std::string& from_filename, const std::string& to_filename) {
    try {
        std::ifstream is(from_filename.c_str(), std::ios::in | std::ios::binary );
        std::ofstream os(to_filename.c_str(), std::ios::out | std::ios::binary );

        // ファイルコピー
        std::istreambuf_iterator<char> iit(is);
        std::istreambuf_iterator<char> end;
        std::ostreambuf_iterator<char> oit(os);
        std::copy( iit, end, oit );
    } catch (...) {
        return false;
    }

    return true;
}

static std::string extractFilename(const char* filename) {
    int len = strlen(filename);
    if (len <= 1) {
        return filename;
    }

    int lastDelpos = -1;
    for (int i = len-1; i >= 0; i--) {
        if (filename[i] == '/' || filename[i] == '\\') {
            lastDelpos = i;
            break;
        }
    }

    if (lastDelpos < 0) {
        return filename;
    }

    return &filename[lastDelpos+1];
}

static void back(const Config& config, CanvasWindow& canvas, Dir& dir) {
    FILE* fp_s = fopen(config.csvFilename.c_str(), "r");
    if (fp_s == NULL) {
        fprintf(stderr, "failed to back: couldn't open %s\n", config.csvFilename.c_str());
        return;
    }

    FILE* fp_d = fopen((config.csvFilename + ".tmp").c_str(), "w");
    if (fp_d == NULL) {
        fprintf(stderr, "failed to back: couldn't open %s\n", (config.csvFilename + ".tmp").c_str());
        fclose(fp_s);
        return;
    }

    char imgfilename[255];
    char xmlfilename[255];
    std::string prevImgFilename;
    std::string prevXmlFilename;

    while(fscanf(fp_s, "%s %s\n", imgfilename, xmlfilename) != EOF) {
        if (0 < prevImgFilename.length() && 0 < prevXmlFilename.length()) {
            // 別ファイルに書き出し
            fprintf(fp_d, "%s %s\n", prevImgFilename.c_str(), prevXmlFilename.c_str());
        }

        prevImgFilename = imgfilename;
        prevXmlFilename = xmlfilename;
    }

    fclose(fp_s);
    fclose(fp_d);

    if (prevImgFilename.length() == 0 || prevXmlFilename.length() == 0) {
        printf("Since no traindata, couldn't back\n");
        remove((config.csvFilename + ".tmp").c_str());
        return;
    }

    // アノテーションファイル(.xml)の削除
    if (remove(xmlfilename)) {
        fprintf(stderr, "failed to back: couldn't remove %s.\n", xmlfilename);
        return;
    }

    // 古い教師データ一覧ファイルの削除
    if (remove(config.csvFilename.c_str())) {
        fprintf(stderr, "failed to back: couldn't remove %s. please remove %s.\n",
            config.csvFilename.c_str(), (config.csvFilename + ".tmp").c_str());
        return;
    }

    // 新しい教師データ一覧ファイルに置き換える
    if (!copyFile((config.csvFilename + ".tmp"), config.csvFilename)) {
        fprintf(stderr, "failed to back: please replace %s to %s.\n",
                (config.csvFilename + ".tmp").c_str(), config.csvFilename.c_str());
        return;
    }

    // tmpの教師データ一覧ファイルの削除
    if (remove((config.csvFilename + ".tmp").c_str())) {
        fprintf(stderr, "failed to back: couldn't remove %s. \n", (config.csvFilename + ".tmp").c_str());
        return;
    }

    // 教師データの画像データを画像データ一覧フォルダに移動させる
    std::string dst = config.images + "backed_" + extractFilename(imgfilename);
    if (!copyFile(imgfilename, dst)) {
        fprintf(stderr, "failed to back: please move %s to %s\n", imgfilename, config.images.c_str());
        return;
    }


    // 教師データの画像データを削除
    if (remove(prevImgFilename.c_str())) {
        fprintf(stderr, "failed to back: couldn't remove %s. \n", prevImgFilename.c_str());
        return;
    }

    printf("back suceess\n");

    canvas.setImage(dst.c_str());
    dir.prev();
}

int main() {

    // 設定ファイルの読み込み
    Config config;
    if (loadConfig(&config)) {
        return -1;
    }

    // フォルダの作成
    mkdir(config.outImages.c_str(), ACCESS_FLAG);
    mkdir(config.outAnnotations.c_str(), ACCESS_FLAG);

    // ディレクトリオブジェクトの生成
    Dir imageDir(config.images, config.ext);

    // パレットウインドウの表示
    PalletWindow pallet(config.labels);

    // キャンパスウインドウの表示
    CanvasWindow canvas;

    // グローバルデータの初期化
    GlobalData data;
    data.dir = &imageDir;
    data.config = &config;
    data.pallet = &pallet;
    data.canvas = &canvas;

    // コールバックの登録
    pallet.setCallback(onClassChanged, &data);
    canvas.setCallback(onSave, &data);

    // クラスの設定
    onClassChanged(&data);

    while (true) {

        if (canvas.isNeededImage()) {
            const std::string& filename = imageDir.next();
            if (filename == "") {
                break;
            }
            canvas.setImage(filename);
        }

        int key = cv::waitKey(1);
        if (key == 27) { // ESC
            break;
        } if (key == 's') { // s
            const std::string& filename = imageDir.next();
            if (filename == "") {
                break;
            }
            canvas.setImage(filename);
        }
    }

    return 0;
}
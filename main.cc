#include "AnnoData.h"
#include "Common.h"
#include "CanvasWindow.h"
#include "PalletWindow.h"
#include <opencv2/opencv.hpp>
#include <json-c/json.h>
#include <sys/stat.h>

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

static void onSave(void* userdata,
        const std::string& filename,
        const cv::Mat& img,
        const std::vector<LabelData>& labels) {

    GlobalData* data = static_cast<GlobalData*>(userdata);

    time_t now = time(NULL);
    std::string baseFilename = format("%llu_%d", (uint64_t)now, data->index++);
    if (0 < data->config->outPrefix.length()) {
        baseFilename = data->config->outPrefix + baseFilename;
    }

    std::string imageFilename = data->config->outImages + baseFilename + data->config->outExt;
    std::string xmlFilename = data->config->outAnnotations + baseFilename + ".xml";
    std::string xml = AnnoData::genXml(*data->config, img.cols, img.rows, imageFilename, labels);
    
    // ソースの画像データを削除
    remove(filename.c_str());

    // 出力フォルダに画像データを保存
    cv::imwrite(imageFilename.c_str(), img);

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
}

int loadConfig(Config* config) {
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
        }
    }

    return 0;
}
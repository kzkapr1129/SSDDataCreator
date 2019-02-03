#include "Common.h"
#include "CanvasWindow.h"
#include "PalletWindow.h"
#include <opencv2/opencv.hpp>
#include <json-c/json.h>
#include <sys/stat.h>

struct GlobalData {
    Dir* dir;
    Config* config;
    PalletWindow* pallet;
    CanvasWindow* canvas;

    GlobalData() : dir(NULL), config(NULL), pallet(NULL), canvas(NULL) {}
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
    // TODO
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
                                config->labels.push_back(json_object_to_json_string(a));
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

    return 0;
}

int main() {

    // 設定ファイルの読み込み
    Config config;
    if (loadConfig(&config)) {
        return -1;
    }

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
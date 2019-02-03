#pragma once

#include "Common.h"
#include <string>
#include <vector>

class AnnoData {
public:
    static std::string genXml(const Config& config,
        int imageWidth, int imageHeight,
        const std::string& imageFilename,
        const std::vector<LabelData>& labels);

private:
    AnnoData() {}
};
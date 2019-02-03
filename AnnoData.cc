#include "AnnoData.h"
#include <stdarg.h>
#include <time.h>

std::string objectXml(const LabelData& label) {
    std::string xmlData;
	xmlData += "\t<object>\n";
	xmlData += format("\t\t<name>%s</name>\n", label.text.c_str());
	xmlData += "\t\t<pose>None</pose>\n"; // TODO
	xmlData += "\t\t<truncated>0</truncated>\n"; // TODO
	xmlData += "\t\t<difficult>0</difficult>\n";
	xmlData += "\t\t<bndbox>\n";
	xmlData += format("\t\t\t<xmin>%d</xmin>\n", label.min.x);
	xmlData += format("\t\t\t<ymin>%d</ymin>\n", label.min.y);
	xmlData += format("\t\t\t<xmax>%d</xmax>\n", label.max.x);
	xmlData += format("\t\t\t<ymax>%d</ymax>\n", label.max.y);
	xmlData += "\t\t</bndbox>\n";
	xmlData += "\t</object>\n";
    return xmlData;
}

std::string AnnoData::genXml(const Config& config,
        int imageWidth, int imageHeight,
        const std::string& imageFilename,
        const std::vector<LabelData>& labels) {
    
    std::string xmlData;

    int flickrid = time(NULL);

    xmlData += "<annotation>\n";
    xmlData += format("\t<folder>%s</folder>\n", config.xmlFolderName.c_str());
    xmlData += format("\t<filename>%s</filename>\n", imageFilename.c_str());
    xmlData += "\t\t<source>\n";
	xmlData += format("\t\t\t<database>%s</database>\n", config.xmlDatabaseName.c_str());
	xmlData += format("\t\t\t<annotation>%s</annotation>\n", config.xmlAnnotationName.c_str());
	xmlData += format("\t\t\t<image>%s</image>\n", config.xmlFolderName.c_str());
	xmlData += format("\t\t\t<flickrid>%ld</flickrid>\n", flickrid);
    xmlData += "\t\t</source>\n";
	xmlData += "\t<owner>\n";
	xmlData += format("\t\t<flickrid>%d</flickrid>\n", flickrid);
	xmlData += format("\t\t<name>%s</name>\n", config.xmlOwnerName.c_str());
	xmlData += "\t</owner>\n";
	xmlData += "\t<size>\n";
	xmlData += format("\t\t<width>%d</width>\n", imageWidth);
	xmlData += format("\t\t<height>%d</height>\n", imageHeight);
	xmlData += "\t\t<depth>3</depth>\n";
	xmlData += "\t</size>\n";
    xmlData += "\t<segmented>0</segmented>\n";

    auto it = labels.begin();
    auto end = labels.end();

    for (; it != end; it++) {
        xmlData += objectXml(*it);
    }

    xmlData += "</annotation>\n";

    return xmlData;
}
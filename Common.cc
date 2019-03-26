#include "Common.h"
#include <dirent.h>

std::string upperString(const std::string& str) {
    std::string upper_string;
    upper_string.resize(str.size());
    std::transform(str.cbegin(), str.cend(), upper_string.begin(), toupper);
    return upper_string;
}

bool endWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    std::string upper = upperString(str);
    return std::equal(std::rbegin(suffix), std::rend(suffix), std::rbegin(upper));
}

Dir::Dir(const std::string& name, const std::string& ext) {
    std::string uext = upperString(ext);

    std::string dirname(name);
    if (dirname.back() != '/') {
        dirname += '/';
    }

    DIR* dp = opendir(dirname.c_str());
    if (dp != NULL) {
        struct dirent* dent;
        do {
            dent = readdir(dp);
            if (dent != NULL && dent->d_type == DT_REG) {
                if (dent->d_name[0] != '.') {
                    std::string filename = dent->d_name;
                    if (endWith(filename, uext)) {
                        mFilelist.push_back(dirname + dent->d_name);
                    }
                }
            }
            
        } while (dent != NULL);
        
        closedir(dp);
    }

    if (0 < mFilelist.size()) {
        mSeek = mFilelist.begin();
    }
}

std::string Dir::next() {
    if (mSeek == mFilelist.end()) {
        return "";
    }

    std::string filename = *mSeek;
    mSeek++;
    return filename;
}

void Dir::prev() {
    mSeek--;
}

std::string format(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(NULL, 0, format, arg);
    va_end(arg);

    char* str = (char*)alloca(sizeof(char) * (len + 1));
    if (str == NULL) {
        printf("internal error: failed alloca %d\n", len + 1);
    }

    va_start(arg, format);
    vsnprintf(str, len + 1, format, arg);
    va_end(arg);
    return str;
}
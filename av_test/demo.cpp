//
// Created by zz on 2023/8/15.
//
#include "iostream"
#include "av_library.h"


int main() {
    const char *videoPath = "test.mp4";
    const char *imgPath = "112.jpg";
    int frameIndex = 4;
    int ret = captureImage(videoPath, imgPath, frameIndex);

    if (ret == 0) {
        printf("image generate success");
    } else {
        printf("image generate fail");
    };

    std::cout << "end";
}



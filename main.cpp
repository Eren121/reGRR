#include <iostream>
#include "regrr.h"
#include <cstdio>

void test()
{
    REGRR_SCOPED("hmmi");
}

int main()
{
    // This sample is Linux-only because of setenv()
    setenv("REGRR_DIR", "tmp", 0);

    cv::Matx<float, 10, 10> a;
    cv::Mat_<cv::Vec2f> b(20, 20);

    REGRR_SCOPED("%s-hi!!%%%%", "main%%");

    REGRR_CREATE_MAT(float, 4, 4, "%s", "hello");
    REGRR_SET_PX(0, 0, 10.0f, "%s", "hello");

    for(int i = 0; i < 10; i++)
    {
        test();
    }

    REGRR_SAVE(a, "%s", "a%%%");
    REGRR_SAVE(b, "%s", "b%%%\\");

    return 0;
}

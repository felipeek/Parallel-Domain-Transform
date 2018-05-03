/*
* Created on 10 September, 2012, 7:48 PM
*/
 
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "ParallelDomainTransform.h"
#include "Util.h"
using namespace cv;
using namespace std;
 
Mat filterCameraFrame(Mat cameraFrame, float* image_data, float* result)
{
    float ss = 20.0f;
    float sr = 1.0f;
    int num_iterations = 3;

    for (int i = 0; i < cameraFrame.rows; ++i)
        for (int j = 0; j < cameraFrame.cols; ++j)
        {
            image_data[4 * cameraFrame.cols * i + 4 * j + 0] = cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 0] / 255.0f;
            image_data[4 * cameraFrame.cols * i + 4 * j + 1] = cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 1] / 255.0f;
            image_data[4 * cameraFrame.cols * i + 4 * j + 2] = cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 2] / 255.0f;
            image_data[4 * cameraFrame.cols * i + 4 * j + 3] = 1.0f;
        }

    start_clock();
    s32 threads = 8;
    s32 x = 1;
    s32 y = 16;
    parallel_domain_transform(image_data, cameraFrame.cols, cameraFrame.rows, 4, ss, sr, num_iterations,
        threads, x, y, result);
    r32 total_time = end_clock();

	print("Total time: %.3f seconds\n", total_time);

    for (int i = 0; i < cameraFrame.rows; ++i)
        for (int j = 0; j < cameraFrame.cols; ++j)
        {
            cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 0] = (u8)roundf(result[4 * cameraFrame.cols * i + 4 * j + 0] * 255.0f);
            cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 1] = (u8)roundf(result[4 * cameraFrame.cols * i + 4 * j + 1] * 255.0f);
            cameraFrame.data[3 * cameraFrame.cols * i + 3 * j + 2] = (u8)roundf(result[4 * cameraFrame.cols * i + 4 * j + 2] * 255.0f);
        }

    return cameraFrame;
}

int main() {
    VideoCapture stream1(0);   //0 is the id of video device.0 if you have only one camera.
    
    stream1.set(CV_CAP_PROP_FRAME_WIDTH,640);
    stream1.set(CV_CAP_PROP_FRAME_HEIGHT,480);

    int width = (int)stream1.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = (int)stream1.get(CV_CAP_PROP_FRAME_HEIGHT);

    print("width: %d, height: %d\n", width, height);

    float *image_data = (float *)malloc(4 * sizeof(float) * width * height);
    float *result = (float *)malloc(4 * sizeof(float) * width * height);

    if (!stream1.isOpened()) { //check if video device has been initialised
        cout << "cannot open camera";
    }
    
    while (true) {
        Mat cameraFrame;
        stream1.read(cameraFrame);
        cameraFrame = filterCameraFrame(cameraFrame, image_data, result);
        imshow("cam", cameraFrame);
        imshow("cam2", cameraFrame);
        if (waitKey(30) >= 0)
        break;
    }

    free(image_data);
    free(result);
    return 0;
}

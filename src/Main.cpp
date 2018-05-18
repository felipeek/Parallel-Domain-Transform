#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include "ParallelDomainTransform.h"
#include "Util.h"
using namespace cv;
using namespace std;
 
#define NUM_THREADS 16
#define Y_BLOCKS 4
#define X_BLOCKS 4
#define S_S 5.0f
#define S_R 0.6f
#define NUM_ITERATIONS 2
#define DETAIL_HIGHLIGHT_FACTOR 1.8f
//#define USE_GAUSSIAN_BLUR_BEFORE_CANNY
#define MERGE_SOBEL_LIMIAR 12
#define MERGE_SOBEL_LINE_INTENSITY 8
#define SOBEL_KSIZE 1
#define SOBEL_SCALE 1
#define SOBEL_DELTA 0

void u8ImageToFloatImage(Mat input, float* output_memory)
{
    for (int i = 0; i < input.rows; ++i)
        for (int j = 0; j < input.cols; ++j)
        {
            output_memory[4 * input.cols * i + 4 * j + 0] = input.data[3 * input.cols * i + 3 * j + 0] / 255.0f;
            output_memory[4 * input.cols * i + 4 * j + 1] = input.data[3 * input.cols * i + 3 * j + 1] / 255.0f;
            output_memory[4 * input.cols * i + 4 * j + 2] = input.data[3 * input.cols * i + 3 * j + 2] / 255.0f;
            output_memory[4 * input.cols * i + 4 * j + 3] = 1.0f;
        }
}

void floatImageToU8Image(Mat input, u8* output_memory)
{
    float* ptr = (float*) input.data;
    size_t elem_step = input.step / sizeof(float);
    for (int i = 0; i < input.rows; ++i)
        for (int j = 0; j < input.cols; ++j)
        {
            output_memory[3 * input.cols * i + 3 * j + 0] = (unsigned char)(roundf(ptr[4 * input.cols * i + 4 * j + 0] * 255.0f));
            output_memory[3 * input.cols * i + 3 * j + 1] = (unsigned char)(roundf(ptr[4 * input.cols * i + 4 * j + 1] * 255.0f));
            output_memory[3 * input.cols * i + 3 * j + 2] = (unsigned char)(roundf(ptr[4 * input.cols * i + 4 * j + 2] * 255.0f));
        }
}

void filterCameraFrame(Mat cameraFrame, Mat* filteredFrame, Mat* detailedFrame,
    float* image_data, float* filteredResult, float* detailedResult)
{
    // Transform input image to float
    u8ImageToFloatImage(cameraFrame, image_data);

    // Filter image
    start_clock();
    parallel_domain_transform(image_data, cameraFrame.cols, cameraFrame.rows, 4, S_S, S_R, NUM_ITERATIONS,
        NUM_THREADS, X_BLOCKS, Y_BLOCKS, filteredResult);
    r32 total_time = end_clock();
	print("Total time: %.3f seconds\n", total_time);

    // Apply R = (I - S) * DHF + I
    for (int i = 0; i < cameraFrame.rows; ++i)
        for (int j = 0; j < cameraFrame.cols; ++j)
        {
            r32 r, g, b;
            r = (image_data[4 * cameraFrame.cols * i + 4 * j + 0] - filteredResult[4 * cameraFrame.cols * i + 4 * j + 0]) *
                DETAIL_HIGHLIGHT_FACTOR + image_data[4 * cameraFrame.cols * i + 4 * j + 0];
            g = (image_data[4 * cameraFrame.cols * i + 4 * j + 1] - filteredResult[4 * cameraFrame.cols * i + 4 * j + 1]) *
                DETAIL_HIGHLIGHT_FACTOR + image_data[4 * cameraFrame.cols * i + 4 * j + 1];
            b = (image_data[4 * cameraFrame.cols * i + 4 * j + 2] - filteredResult[4 * cameraFrame.cols * i + 4 * j + 2]) *
                DETAIL_HIGHLIGHT_FACTOR + image_data[4 * cameraFrame.cols * i + 4 * j + 2];
            
            r = MIN(1.0f, MAX(0.0f, r));
            g = MIN(1.0f, MAX(0.0f, g));
            b = MIN(1.0f, MAX(0.0f, b));

            detailedResult[4 * cameraFrame.cols * i + 4 * j + 0] = r;
            detailedResult[4 * cameraFrame.cols * i + 4 * j + 1] = g;
            detailedResult[4 * cameraFrame.cols * i + 4 * j + 2] = b;
        }

    // Fill Mats
    *filteredFrame = cv::Mat(cameraFrame.rows, cameraFrame.cols, CV_32FC4, filteredResult);
    *detailedFrame = cv::Mat(cameraFrame.rows, cameraFrame.cols, CV_32FC4, detailedResult);
}

void mergeSobel(Mat sobel, Mat out)
{
    for (int i = 0; i < out.rows; ++i)
        for (int j = 0; j < out.cols; ++j)
        {
            unsigned char c = sobel.data[1 * sobel.cols * i + 1 * j + 0];

            if (c < MERGE_SOBEL_LIMIAR) continue;

            int x = j;
            int y = i;
            out.data[3 * out.cols * y + 3 * x + 0] = MIN(out.data[3 * out.cols * y + 3 * x + 0] + MERGE_SOBEL_LINE_INTENSITY * c, 255);
            out.data[3 * out.cols * y + 3 * x + 1] = MIN(out.data[3 * out.cols * y + 3 * x + 1] + MERGE_SOBEL_LINE_INTENSITY * c, 255);
            out.data[3 * out.cols * y + 3 * x + 2] = MIN(out.data[3 * out.cols * y + 3 * x + 2] + MERGE_SOBEL_LINE_INTENSITY * c, 255);
        }
}

int main() {
    VideoCapture stream1(0);   //0 is the id of video device.0 if you have only one camera.
    
    stream1.set(CV_CAP_PROP_FRAME_WIDTH,640);
    stream1.set(CV_CAP_PROP_FRAME_HEIGHT,480);

    int width = (int)stream1.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = (int)stream1.get(CV_CAP_PROP_FRAME_HEIGHT);

    // Pre-alloc memory
    float *image_data = (float *)malloc(4 * sizeof(float) * width * height);
    float *filteredResult = (float *)malloc(4 * sizeof(float) * width * height);
    float *detailedResult = (float *)malloc(4 * sizeof(float) * width * height);
    unsigned char *filteredResultWithCanny = (unsigned char *)malloc(3 * sizeof(unsigned char) * width * height);

    if (!stream1.isOpened()) { //check if video device has been initialised
        cout << "cannot open camera";
    }

    // Create windows
    namedWindow("input");
    moveWindow("input", 0,0);
    namedWindow("output1");
    moveWindow("output1", width,0);
    namedWindow("output2");
    moveWindow("output2", width,100);
    namedWindow("output3");
    moveWindow("output3", width,300);

    while (true) {
        Mat cameraFrame, filteredFrame, detailedFrame, sobelFrame, blurFrame, blurFrameGray;
        stream1.read(cameraFrame);
        filterCameraFrame(cameraFrame, &filteredFrame, &detailedFrame, image_data, filteredResult, detailedResult);

        floatImageToU8Image(filteredFrame, filteredResultWithCanny);
        Mat filteredFrameWithSobel = cv::Mat(filteredFrame.rows, filteredFrame.cols, CV_8UC3, filteredResultWithCanny);

#ifdef USE_GAUSSIAN_BLUR_BEFORE_CANNY
        cv::GaussianBlur(filteredFrameWithSobel, blurFrame, cv::Size(7, 7), 2.0);
#else
        blurFrame = filteredFrameWithSobel;
#endif

        cvtColor(blurFrame, blurFrameGray, COLOR_BGR2GRAY);

        Mat grad_x, grad_y;
        Mat abs_grad_x, abs_grad_y;

        Sobel(blurFrameGray, grad_x, CV_16S, 1, 0, SOBEL_KSIZE, SOBEL_SCALE, SOBEL_DELTA, BORDER_DEFAULT);
        Sobel(blurFrameGray, grad_y, CV_16S, 0, 1, SOBEL_KSIZE, SOBEL_SCALE, SOBEL_DELTA, BORDER_DEFAULT);
        // converting back to CV_8U
        convertScaleAbs(grad_x, abs_grad_x);
        convertScaleAbs(grad_y, abs_grad_y);
        addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, sobelFrame);

        mergeSobel(sobelFrame, filteredFrameWithSobel);

        imshow("input", cameraFrame);
        imshow("output1", filteredFrame);
        imshow("output2", detailedFrame);
        imshow("output3", filteredFrameWithSobel);
        if (waitKey(30) >= 0)
        break;
    }

    // Free memory
    free(image_data);
    free(filteredResult);
    free(detailedResult);
    free(filteredResultWithCanny);
    return 0;
}

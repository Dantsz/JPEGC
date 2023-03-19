// JPEGC.cpp : Defines the entry point for the application.
//

#include "JPEGC.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
using namespace std;

int main()
{
	cout << "'Hello CMake." << endl;
	cv::Mat_<uchar> img(256, 256, static_cast<uchar>(0));
	cv::imshow("a", img);
	
	return 0;
}

// JPEGC.cpp : Defines the entry point for the application.
//

#include "JPEGC.h"
#include <opencv2/opencv.hpp>


int main()
{
	std::cout << "'Hello CMake." << std::endl;
	cv::Mat_<uchar> img(256, 256, static_cast<uchar>(0));
	cv::imshow("a", img);
	cv::waitKey();
	return 0;
}

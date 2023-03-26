#include <tuple>
#include <opencv2/opencv.hpp>
#include <array>
export module imageproc;
using SIMG = cv::Mat_<unsigned char>;
using TIMG = cv::Mat_<cv::Vec3b>;

export std::tuple<SIMG, SIMG, SIMG> transform_bgr_to_yuv_split(const TIMG img)
{
	auto yuv = img.clone();
	cv::cvtColor(img, yuv, cv::COLOR_BGR2YUV);
	std::array<SIMG, 3> splits;
	cv::split(yuv, splits.data());
	return std::make_tuple(splits[0], splits[1], splits[2]);
}
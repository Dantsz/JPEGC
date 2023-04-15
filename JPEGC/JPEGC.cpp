﻿#include <opencv2/opencv.hpp>
#include <argumentum/argparse.h>
#include <string>
#include <format>
using namespace argumentum;

import imageproc;
struct defaults
{
	static constexpr std::string_view default_destination_path = "./res.bjpeg";
};




int main(int argc, char** argv)
{
	std::string src_path{};
	std::string dst_path{};
	bool decompress = false;
	auto parser = argument_parser{};
	auto params = parser.params();
	params.add_parameter(decompress, "--decompress", "-d").nargs(0).help("Compress or decompress (Default: compress)");
	params.add_parameter(src_path, "src").nargs(1).metavar("SrcPath").help("Path to source image");
	params.add_parameter(dst_path, "dst").minargs(0).metavar("DstPath").absent(defaults::default_destination_path.data()).help("Path to destination image");

	if (!parser.parse_args(argc, argv, 1))
		return 1;
	std::cout << std::format("Compressing/Decompressing({}) {} -> {}",decompress,src_path,dst_path) << std::endl;

	cv::Mat_<cv::Vec3b> img = cv::imread(src_path);
	const auto [y, u, v] = transform_bgr_to_yuv_split(img);
	const auto transformed_img = transform_yuv_to_bgr_combine(std::make_tuple(y, u, v));
	cv::imshow("img", transformed_img);
	const auto compressed_y = jpeg_compress(y);
	const auto compressed_u = jpeg_compress(u);
	const auto compressed_v = jpeg_compress(v);

	const auto decompressed_y = jpeg_decompress(compressed_y);
	const auto decompressed_u = jpeg_decompress(compressed_u);
	const auto decompressed_v = jpeg_decompress(compressed_v);
	const auto decompressed_img = transform_yuv_to_bgr_combine(std::make_tuple(decompressed_y, decompressed_u, decompressed_v));
	cv::imshow("img_compressed", decompressed_img);
	//cv::imshow("Y", y);
	///*cv::imshow("U", u);
	//cv::imshow("V", v);*/

	//
	//const auto compressed = jpeg_compress(y);
	//const auto new_Y = jpeg_decompress(compressed);



	//cv::imshow("new_Y", new_Y);

	cv::waitKey();
	return 0;
}

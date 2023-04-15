#include <opencv2/opencv.hpp>
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



	cv::imshow("Y", y);
	/*cv::imshow("U", u);
	cv::imshow("V", v);*/

	
	
	const auto chunks_of_Y = chunk_image(y);
		const auto fdct = FDCT(chunks_of_Y);
			const auto quant = quantize(fdct);
				const auto zz = zz_encode(quant);
				const auto rl = row_length_encode(zz);
				const auto drl = row_length_decode(rl);
				const auto d_zz = zz_decode(drl);
			const auto rev_quant = dequantize(d_zz);
		const auto rev_fdct = rev_FDCT(rev_quant);
	const auto new_Y = reconstruct_image(rev_fdct, y.rows, img.cols);


	cv::imshow("new_Y", new_Y);

	cv::waitKey();
	return 0;
}

// JPEGC.cpp : Defines the entry point for the application.
//

#include <opencv2/opencv.hpp>
#include <argumentum/argparse.h>
#include <string>
#include <format>
using namespace argumentum;

struct defaults
{
	static constexpr std::string_view default_destination_path = "./res.bjpeg";
};




int main(int argc, char** argv)
{
	std::string src_path{};
	std::string dst_path{};

	auto parser = argument_parser{};
	auto params = parser.params();

	params.add_parameter(src_path, "src").nargs(1).metavar("SrcPath").help("Path to source image");
	params.add_parameter(dst_path, "dst").minargs(0).metavar("DstPath").absent(defaults::default_destination_path.data()).help("Path to destination image");

	if (!parser.parse_args(argc, argv, 1))
		return 1;
	std::cout << std::format("Compressing {} -> {}",src_path,dst_path);
	return 0;
}

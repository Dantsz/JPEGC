#include <opencv2/opencv.hpp>
#include <argumentum/argparse.h>
#include <string>
#include <format>
#include <future>
#include <thread>
#include <alpaca/alpaca.h>
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

	if (!decompress)
	{
		std::cout << std::format("Compressing {} -> {}", src_path, dst_path) << std::endl;
		cv::Mat_<cv::Vec3b> img = cv::imread(src_path);
		const auto [y, u, v] = transform_bgr_to_yuv_split(img);
		const auto transformed_img = transform_yuv_to_bgr_combine(std::make_tuple(y, u, v));
		cv::imshow("img", transformed_img);
		/*
		* This would have been so much better if I had a 'then' member function for futures
		*/
		auto f_y = std::async(std::launch::async, [&y]() { return jpeg_compress(y); });
		auto f_u = std::async(std::launch::async, [&u]() { return jpeg_compress(u); });
		auto f_v = std::async(std::launch::async, [&v]() { return jpeg_compress(v); });

		const auto compressed_data = std::make_tuple(f_y.get(), f_u.get(), f_v.get());
		std::vector<uint8_t> data;
		auto bytes_written = alpaca::serialize(compressed_data, data);
		std::ofstream file(dst_path, std::ios::binary);
		file.write(reinterpret_cast<const char*>(data.data()), data.size());
		file.close();

	}
	else
	{
		std::cout << std::format("Decompressing {} -> {}", src_path, dst_path) << std::endl;
	
		std::ifstream file(src_path, std::ios::binary);
		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<uint8_t> data(size);
		if (file.read((char*)&data[0], size))
		{
			std::error_code ec;
			auto [compressed_y, compressed_u, compressed_v] = alpaca::deserialize<std::tuple<JPECCompressedData, JPECCompressedData, JPECCompressedData>>(data,ec);
			if (!ec) {
				auto fd_y = std::async(std::launch::async, [&compressed_y]() { return jpeg_decompress(compressed_y); });
				auto fd_u = std::async(std::launch::async, [&compressed_u]() { return jpeg_decompress(compressed_u); });
				auto fd_v = std::async(std::launch::async, [&compressed_v]() { return jpeg_decompress(compressed_v); });

				const auto decompressed_y = fd_y.get();
				const auto decompressed_u = fd_u.get();
				const auto decompressed_v = fd_v.get();

				const auto decompressed_img = transform_yuv_to_bgr_combine(std::make_tuple(decompressed_y, decompressed_u, decompressed_v));
				cv::imshow("img_compressed", decompressed_img);
				cv::waitKey();
			}
			else
			{
				std::cout << "Failed to decompress image" << '\0';
				return -1;
			}
		}
		file.close();
	}
	return 0;
}

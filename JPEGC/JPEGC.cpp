#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <argumentum/argparse.h>
#include <string>
#include <format>
#include <future>
#include <thread>
#include <optional>
#include <alpaca/alpaca.h>
using namespace argumentum;

import imageproc;
struct defaults
{
	static constexpr std::string_view default_destination_path = "./res.bjpeg";
	static constexpr std::string_view default_destination_decompression_path = "./res.bmp";
};




int main(int argc, char** argv)
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);
	std::string src_path{};
	std::optional<std::string> dst_path{};
	bool decompress = false;
	auto parser = argument_parser{};
	auto params = parser.params();
	params.add_parameter(decompress, "--decompress", "-d").nargs(0).help("Compress or decompress (Default: compress)");
	params.add_parameter(src_path, "src").nargs(1).metavar("SrcPath").help("Path to source image");
	params.add_parameter(dst_path, "dst").minargs(0).metavar("DstPath").absent("").help("Path to destination image");

	if (!parser.parse_args(argc, argv, 1))
		return 1;

	if (!decompress)
	{
		if (!dst_path.has_value())
		{
			dst_path = defaults::default_destination_path;
		}
		std::cout << std::format("Compressing {} -> {}", src_path, dst_path.value()) << std::endl;
		cv::Mat_<cv::Vec3b> img = cv::imread(src_path);
		const auto [y, u, v] = transform_bgr_to_yuv_split(img);
		const auto transformed_img = transform_yuv_to_bgr_combine(std::make_tuple(y, u, v));
		/*
		* This would have been so much better if I had a 'then' member function for futures
		*/
		auto f_y = std::async(std::launch::async, [&y]() { return jpeg_compress(y); });
		auto f_u = std::async(std::launch::async, [&u]() { return jpeg_compress(u,0.5); });
		auto f_v = std::async(std::launch::async, [&v]() { return jpeg_compress(v,0.5); });
		
		const auto compressed_data = std::make_tuple(f_y.get(), f_u.get(), f_v.get());
		std::vector<uint8_t> data;
		auto bytes_written = alpaca::serialize(compressed_data, data);
		std::ofstream file(dst_path.value(), std::ios::binary);
		file.write(reinterpret_cast<const char*>(data.data()), data.size());
		file.close();

	}
	else
	{
		if (!dst_path.has_value())
		{
			dst_path = defaults::default_destination_decompression_path;
		}
		std::cout << std::format("Decompressing {} -> {}", src_path, dst_path.value()) << std::endl;

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
				const auto original_size_x = std::get<1>(compressed_y);
				const auto original_size_y = std::get<0>(compressed_y);
				auto fd_y = std::async(std::launch::async, [&compressed_y, &original_size_x, &original_size_y]() { return jpeg_decompress(compressed_y, original_size_x, original_size_y); });
				auto fd_u = std::async(std::launch::async, [&compressed_u, &original_size_x, &original_size_y]() { return jpeg_decompress(compressed_u, original_size_x, original_size_y); });
				auto fd_v = std::async(std::launch::async, [&compressed_v, &original_size_x, &original_size_y]() { return jpeg_decompress(compressed_v, original_size_x, original_size_y); });

				const auto decompressed_y = fd_y.get();
				const auto decompressed_u = fd_u.get();
				const auto decompressed_v = fd_v.get();

				const auto decompressed_img = transform_yuv_to_bgr_combine(std::make_tuple(decompressed_y, decompressed_u, decompressed_v));
				cv::imwrite(dst_path.value(), decompressed_img);
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


#include <tuple>
#include <opencv2/opencv.hpp>
#include <array>
#include <valarray>
#include <vector>
#include <functional>
#include <numbers>
#include <cstdint>

export module imageproc;
using SIMG = cv::Mat_<unsigned char>;
using SSIMG = cv::Mat_<int>;
using SCIMG = cv::Mat_<int8_t>;
using FIMG = cv::Mat_<float>;
using TIMG = cv::Mat_<cv::Vec3b>;

export std::tuple<SIMG, SIMG, SIMG> transform_bgr_to_yuv_split(const TIMG img)
{
	auto yuv = img.clone();
	cv::cvtColor(img, yuv, cv::COLOR_BGR2YUV);
	std::array<SIMG, 3> splits;
	cv::split(yuv, splits.data());
	return std::make_tuple(splits[0], splits[1], splits[2]);
}

export template<typename T>
bool isInside(const cv::Mat_ <T>& mat, int i, int j)
{
	return i >= 0 && j >= 0 && i < mat.rows && j < mat.cols;
}

export template<int ROWS = 8, int COLS = 8>
std::vector<std::vector<SIMG>> chunk_image(const SIMG& src_image)
{
	const int chunk_mat_rows = src_image.rows / ROWS + (src_image.rows % ROWS == 0 ? 0 : 1);
	const int chunk_mat_cols = src_image.cols / COLS + (src_image.cols % COLS == 0 ? 0 : 1);
	std::vector<std::vector<SIMG>> chunk_mat{};
	chunk_mat.resize(chunk_mat_rows);
	for (auto i = 0; i < chunk_mat_rows; i++)
	{
		for (auto j = 0; j < chunk_mat_cols; j++)
		{
			chunk_mat[i].emplace_back(ROWS, COLS);
		}
	}
	for (int i = 0; i < src_image.rows; i++)
	{
		for (int j = 0; j < src_image.cols; j++)
		{
			chunk_mat[i / ROWS][j / COLS]((i % ROWS), (j % COLS)) = src_image(i, j);
		}
	}
	return chunk_mat;
}

export template<int ROWS = 8, int COLS = 8>
SIMG reconstruct_image(const std::vector<std::vector<SIMG>>& chunks, size_t height ,size_t width)
{
	if (chunks.empty() || chunks[0].empty())
	{
		return SIMG(0, 0);
	}
	SIMG img(ROWS * chunks.size(), COLS * chunks[0].size());
	img.setTo(0);
	for (int i = 0; i < chunks.size(); i++)
	{
		for (int j = 0; j < chunks[i].size(); j++)
		{
			for (int ii = 0; ii < chunks[i][j].rows; ii++)
			{
				for (int jj = 0; jj < chunks[i][j].cols; jj++)
				{
					img(i * ROWS + ii, j * COLS + jj) = chunks[i][j](ii, jj);
				}
			}
		}
	}
	cv::Rect crop_region(0,0, width, height);
	return img(crop_region);

}

template<typename IN , typename OUT>
std::vector <std::vector<cv::Mat_<OUT>>> transform_chunk(const std::vector <std::vector<cv::Mat_<IN>>>& in_data,const std::function<OUT(IN,int,int)>& transform_func )
{
	std::vector<std::vector<cv::Mat_<OUT>>> out_data{};
	out_data.resize(in_data.size());
	for (auto i = 0; i < in_data.size(); i++)
	{
		for (auto j = 0; j < in_data[i].size(); j++)
		{
			out_data[i].emplace_back(in_data[i][j].rows, in_data[i][j].cols);
			for (int ii = 0; ii < in_data[i][j].rows; ii++)
			{
				for (int jj = 0; jj < in_data[i][j].cols; jj++)
				{
					out_data[i][j](ii, jj) = transform_func(in_data[i][j](ii, jj),ii,jj);
				}
			}
		}
	}
	return out_data;
}


template<typename IN, typename OUT>
std::vector <std::vector<cv::Mat_<OUT>>> transform_chunk(const std::vector <std::vector<cv::Mat_<IN>>>& in_data, const std::function< cv::Mat_<OUT>(const cv::Mat_ <IN>&)>& transform_func)
{
	std::vector<std::vector<cv::Mat_<OUT>>> out_data{};
	out_data.resize(in_data.size());
	for (auto i = 0; i < in_data.size(); i++)
	{
		for (auto j = 0; j < in_data[i].size(); j++)
		{
			out_data[i].emplace_back(transform_func(in_data[i][j]));
		}
	}
	return out_data;
}

export std::vector<std::vector<FIMG>> FDCT(const std::vector<std::vector<SIMG>>& data)
{
	//subtracting
	std::vector<std::vector<SSIMG>> subtracted_values = transform_chunk<unsigned char, int>(data, [](unsigned char unsigned_value,int,int) { return static_cast<int>(unsigned_value - 128); });
	std::vector<std::vector<FIMG>> T = transform_chunk<int, float>(subtracted_values, [](SSIMG chunk) -> SSIMG {
		FIMG FDCT_chunk(chunk.rows, chunk.cols);
		for (auto i = 0; i < FDCT_chunk.rows; i++)
		{
			for (auto j = 0; j < FDCT_chunk.cols; j++)
			{
				FDCT_chunk(i, j) = 0.0f;
				for (auto ii = 0; ii < chunk.rows; ii++)
				{
					for (auto jj = 0; jj < chunk.cols; jj++)
					{
						FDCT_chunk(i, j) += static_cast<float>(chunk(ii, jj)) * std::cos(((2 * ii + 1) * i * std::numbers::pi_v<float>) /16.0f)
																			  * std::cos(((2 * jj + 1) * j * std::numbers::pi_v<float>) /16.0f);
					}
				}
				const float alpha_i = i == 0 ? std::sqrt(1.0f / 8.0f) : std::sqrt(2.0f / 8.0f);
				const float alpha_j = j == 0 ? std::sqrt(1.0f / 8.0f) : std::sqrt(2.0f / 8.0f);
				FDCT_chunk(i, j) *= alpha_i * alpha_j;
			}
		}
		return FDCT_chunk;
	});
	return T;
}
export std::vector<std::vector<SIMG>> rev_FDCT(const std::vector<std::vector<FIMG>>& data)
{	
	std::vector < std::vector<SSIMG>> subtracted_values = transform_chunk<float, int>(data, 
		[](FIMG float_chunk)->SSIMG 
		{
			SSIMG subtracted_chunk(float_chunk.rows, float_chunk.cols);
			for (auto i = 0; i < subtracted_chunk.rows; i++)
			{
				
				for (auto j = 0; j < subtracted_chunk.cols; j++) 
				{
					float acc = 0.0f;
					for (auto ii = 0; ii < float_chunk.rows; ii++)
					{
						for (auto jj = 0; jj < float_chunk.cols; jj++)
						{
							const float alpha_i = ii == 0 ? std::sqrt(1.0f / 8.0f) : std::sqrt(2.0f / 8.0f);
							const float alpha_j = jj == 0 ? std::sqrt(1.0f / 8.0f) : std::sqrt(2.0f / 8.0f);
							acc +=  alpha_i * alpha_j * float_chunk(ii, jj) * std::cos(((2 * i + 1) * ii * std::numbers::pi_v<float>)/16.0f)
																			* std::cos(((2 * j + 1) * jj * std::numbers::pi_v<float>)/16.0f);
						}
					}
					subtracted_chunk(i, j) =  static_cast<char>(acc);
				}
			}
			return subtracted_chunk;
		});





	std::vector<std::vector<SIMG>> added_values = transform_chunk<int, unsigned char>(subtracted_values, [](const SSIMG& signed_values)
		{ 
			SIMG transformed_chunk(signed_values.rows, signed_values.cols);
			for (int i = 0; i < signed_values.rows; i++)
			{
				for (int j = 0; j < signed_values.cols; j++)
				{
					transformed_chunk(i,j)  = static_cast<unsigned char>(signed_values(i,j) + 128);
				}
			}
			return transformed_chunk;
		
		});
	return added_values;
}
export std::vector<std::vector<SCIMG>> quantize(const std::vector<std::vector<FIMG>>& fdct_data)
{
	const int quantization_table[][8] = { {16,11,10,16,24,40, 51,61},
											{12,12,14,19,26,58, 60,55},
											{14,13,16,24,40,57, 69,56},
											{14,17,22,29,51,87, 80,62},
											{18,22,37,56,68,109,103,77},
											{24,35,55,64,81,104,113,92},
											{49,64,78,87,103,121,120,101},
											{72,92,95,98,112,100,103,99} };
	std::vector<std::vector<SCIMG>> quantized_data = transform_chunk<float, int8_t>(fdct_data, [&](float fdct_value, int i, int j)->int {return static_cast<int8_t>(static_cast<int>(fdct_value) / quantization_table[i][j]); });
	return quantized_data;
}

export std::vector<std::vector<FIMG>> dequantize(const std::vector<std::vector<SCIMG>>& quantized_data)
{
	const float quantization_table[][8] = { {16,11,10,16,24,40, 51,61},
											{12,12,14,19,26,58, 60,55},
											{14,13,16,24,40,57, 69,56},
											{14,17,22,29,51,87, 80,62},
											{18,22,37,56,68,109,103,77},
											{24,35,55,64,81,104,113,92},
											{49,64,78,87,103,121,120,101},
											{72,92,95,98,112,100,103,99} };
	std::vector<std::vector<FIMG>> fdct_data = transform_chunk<int8_t, float>(quantized_data, [&](int8_t quantized_value, int i, int j) -> float { return static_cast<float>(quantized_value) * quantization_table[i][j]; });
	return fdct_data;
}

export std::tuple<std::vector<std::array<int8_t,64>>,size_t,size_t> zz_encode(const std::vector<std::vector<SCIMG>>& quantized_data)
{
	const int zigzag_LUT[] = { 0, 1, 8, 16, 9, 2, 3, 10,
						17, 24, 32, 25, 18, 11, 4, 5,
						12, 19, 26, 33, 40, 48, 41, 34,
						27, 20, 13, 6, 7, 14, 21, 28,
						35, 42, 49, 56, 57, 50, 43, 36,
						29, 22, 15, 23, 30, 37, 44, 51,
						58, 59, 52, 45, 38, 31, 39, 46,
						53, 60, 61, 54, 47, 55, 62, 63 };

	std::vector< std::array<int8_t,64>> data{};

	for (auto i = 0; i < quantized_data.size(); i++)
	{
		for (auto j = 0; j < quantized_data[i].size(); j++)
		{
			data.emplace_back();
			for (auto ii = 0; ii < quantized_data[i][j].rows * quantized_data[i][j].cols; ii++)
			{
				const int row = zigzag_LUT[ii] / 8;
				const int col = zigzag_LUT[ii] % 8;
				data.back()[ii] = quantized_data[i][j](row, col);
			}
		}
	}
	return std::make_tuple(data, quantized_data.size(), quantized_data[0].size());
}
export std::vector<std::vector<SCIMG>> zz_decode(const std::tuple<std::vector<std::array<int8_t, 64>>, size_t, size_t>& args)
{
	const std::vector< std::array<int8_t, 64>>& encoded_data = std::get<0>(args);
	auto rows = std::get<1>(args);
	auto cols = std::get<2>(args);
	const int zigzag_LUT[] = { 0, 1, 8, 16, 9, 2, 3, 10,
					17, 24, 32, 25, 18, 11, 4, 5,
					12, 19, 26, 33, 40, 48, 41, 34,
					27, 20, 13, 6, 7, 14, 21, 28,
					35, 42, 49, 56, 57, 50, 43, 36,
					29, 22, 15, 23, 30, 37, 44, 51,
					58, 59, 52, 45, 38, 31, 39, 46,
					53, 60, 61, 54, 47, 55, 62, 63 };
	std::vector<std::vector<SCIMG>> quantized_data{};
	quantized_data.resize(rows);
	for (auto i = 0; i < rows; i++)
	{
		for (auto j = 0; j < cols; j++)
		{
			quantized_data[i].emplace_back(8, 8);
			for (auto ii = 0; ii < quantized_data[i][j].rows * quantized_data[i][j].cols; ii++)
			{
				const int row = zigzag_LUT[ii] / 8;
				const int col = zigzag_LUT[ii] % 8;
				quantized_data[i][j](row, col) = encoded_data[cols * i + j][ii];
			}
		}
	}
	return quantized_data;
}

export std::tuple<size_t, size_t,std::vector<std::vector<std::tuple<int8_t, uint8_t>>>> row_length_encode(const std::tuple<std::vector<std::array<int8_t, 64>>, size_t, size_t>& args)
{
	std::vector < std::vector < std::tuple<int8_t, uint8_t>>> row_lengths{};
	
	const auto rows = std::get<1>(args);
	const auto cols = std::get<2>(args);
	const auto& rows_data = std::get<0>(args);
	for (const auto& row : rows_data)
	{
		row_lengths.emplace_back();
		int8_t value = row[0];
		int8_t count = 1;
		for (int i = 1; i < row.size(); i++)
		{
			if (row[i] != value || i + 1 == row.size())
			{
				row_lengths.back().emplace_back(value, count);
				value = row[i];
				count = 1;
			}
			else
			{
				count++;
			}
		}
	}
	return std::make_tuple(rows,cols,row_lengths);
}
export  std::tuple<std::vector<std::array<int8_t, 64>>, size_t, size_t> row_length_decode(const std::tuple<size_t, size_t,std::vector<std::vector<std::tuple<int8_t, uint8_t>>>>& args)
{
	const auto rows = std::get<0>(args);
	const auto cols = std::get<1>(args);
	const auto rows_lengths = std::get<2>(args);
	std::vector<std::array<int8_t, 64>> rows_data{};
	rows_data.resize(rows_lengths.size());
	for (auto j = 0 ;j < rows_lengths.size(); j++)
	{
		size_t index = 0;
		for (const auto [value,count] : rows_lengths[j])
		{
			for (int i = 0; i < count; i++)
			{
				rows_data[j][index + i] = value;
			}
			index += count;
		}
	}
	return std::make_tuple(rows_data, rows, cols);
}
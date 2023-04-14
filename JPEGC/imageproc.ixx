
#include <tuple>
#include <opencv2/opencv.hpp>
#include <array>
#include <valarray>
#include <vector>
#include <functional>
#include <numbers>

export module imageproc;
using SIMG = cv::Mat_<unsigned char>;
using SSIMG = cv::Mat_<int>;
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
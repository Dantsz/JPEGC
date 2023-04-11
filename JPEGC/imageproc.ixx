#include <tuple>
#include <opencv2/opencv.hpp>
#include <array>
#include <valarray>
#include <vector>
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
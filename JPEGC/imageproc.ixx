#include <tuple>
#include <opencv2/opencv.hpp>
#include <array>
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
		chunk_mat[i].resize(chunk_mat_cols, SIMG(ROWS, COLS));
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
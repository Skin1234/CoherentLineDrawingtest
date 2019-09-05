// CoherentLineDrawingtest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*#include "pch.h"*/
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <string.h>
#include <opencv2/highgui.hpp>
#include <math.h>
#include <fstream>
#define PI 3.1415926

#ifdef _DEBUG
	#pragma comment(lib, "opencv_world411d.lib")
#else
	#pragma comment(lib, "opencv_world411.lib")
#endif

typedef cv::Mat Matrix1d;
typedef cv::Mat Matrix2d;

const double mysoble_template_x[3*3] = {-1,0,1,-2,0,2,-1,0,1};
const double mysoble_template_y[3*3] = { -1,-2,-1,0,0,0,1,2,1 };

void convolution_kernel(const double template_In[],int size,cv::Mat &template_Out)
{
	int wh = sqrt(size);
	int in_index = 0;
	for (auto r = 0; r < wh; r++)
	{
		for (auto c = 0; c < wh; c++)
		{
			template_Out.at<double>(r, c) = template_In[in_index++];
		}
	}
}

void Convolution_d(const cv::Mat &gray_img_In, const cv::Mat &template_kernel, cv::Mat &gradient_img_Out)
{
	int kernel = (template_kernel.rows - 1) / 2;
	for (auto r = kernel; r < gray_img_In.rows - kernel; r++)
	{
		for (auto c = kernel; c < gray_img_In.cols - kernel; c++)
		{
			double sum_val = 0.0;
			for (auto r_kernel = 0; r_kernel < template_kernel.rows; r_kernel++)
			{
				for (auto c_kernel = 0; c_kernel < template_kernel.cols; c_kernel++)
				{
					double val1 = gray_img_In.at<double>(r + r_kernel - kernel, c + c_kernel - kernel);
					double val2 = template_kernel.at<double>(r_kernel, c_kernel);
					sum_val += val1 * val2;
				}
			}
			gradient_img_Out.at<double>(r, c) = sum_val;
		}
	}
}

void template_gos(int template_kernel, double sigma, cv::Mat &result)
{
	int size = 2 * template_kernel + 1;
	cv::Mat gaussian_kernel_sigma(size, size, CV_64FC1, cv::Scalar(0));

	double sum = 0.0;
	for (auto r_kernel = -template_kernel; r_kernel <= template_kernel; r_kernel++)
	{
		for (auto c_kernel = -template_kernel; c_kernel <= template_kernel; c_kernel++)
		{
			double val = std::exp(-(r_kernel* r_kernel+c_kernel*c_kernel) / (2 * sigma*sigma)) / (std::sqrt(2 * PI*sigma));
 			gaussian_kernel_sigma.at<double>(r_kernel+ template_kernel, c_kernel + template_kernel) = val;
 			sum += val;
		}
		
	}
	for (auto r_kernel = 0; r_kernel < size; r_kernel++)
	{
		for (auto c_kernel = 0; c_kernel <size; c_kernel++)
		{
			gaussian_kernel_sigma.at<double>(r_kernel, c_kernel) = gaussian_kernel_sigma.at <double>(r_kernel, c_kernel) / sum;
		}

	}

	result = gaussian_kernel_sigma;
}

double myamplitude(double x, double y)
{
	return sqrt(x * x + y * y);
}

void CalculateMag(const Matrix2d &In, Matrix1d &Out)
{
	for (auto r = 0; r < In.rows; r++)
	{
		for (auto c = 0; c < In.cols; c++)
		{
			cv::Vec2d v = In.at<cv::Vec2d>(r, c);
			double val = sqrt(v[0] * v[0] + v[1] * v[1]);
			Out.at<double>(r, c) = val;
		}
	}
}

double myPhi(const cv::Vec2d &t_cur_x, const cv::Vec2d &t_cur_y)
{
	return t_cur_x.dot(t_cur_y) > 0 ? 1 : -1;
}

double myWs(cv::Point tx, cv::Point ty, int r)
{
	return sqrt((tx.x - ty.x)*(tx.x - ty.x) + (tx.y - ty.y)*(tx.y - ty.y)) < r ? 1 : 0;
}

double myWm(double gy, double gx, double H)
{
	return 0.5 * (1.0 + std::tanh(H*(gy - gx)));
}

double myWd(const cv::Vec2d t_cur_x, const cv::Vec2d t_cur_y)
{
	return abs(t_cur_x.dot(t_cur_y));
}

void write_file(const cv::Mat &matrix, const std::string &filename)
{
	std::ofstream fout(filename);
	fout << matrix.cols << ' ' << matrix.rows << '\n' << '\r';

	for (auto r = 0; r < matrix.rows; r++)
	{
		for (auto c = 0; c < matrix.cols; c++)
		{
			cv::Vec2d v = matrix.at<cv::Vec2d>(r, c);
			fout <</*r << "\t" <<c <<":"<<*/ v[0] << "\t" << v[1] << "\n";
		}
	}
	fout.close();
}
 
void compute_TNewX(const Matrix2d &tangent_coordinate, const Matrix1d &gradient_amplitude, int kernel, cv::Mat &tangent_new)
{
	//求t_cur_y
	cv::Vec2d t_cur_x;
	cv::Vec2d t_cur_y;
	
	for (auto r = kernel; r < tangent_coordinate.rows - kernel; r++)
	{
		for (auto c = kernel; c < tangent_coordinate.cols - kernel; c++)
		{
			t_cur_x = tangent_coordinate.at<cv::Vec2d>(r, c); 

			cv::Vec2d t_new_x;
			double sum = 0.0;
			for (auto r_kernel = 0; r_kernel <= 2 * kernel; r_kernel++)
			{
				for (auto c_kernel = 0; c_kernel <= 2 * kernel; c_kernel++)
				{
					cv::Point ty(c + c_kernel - kernel, r + r_kernel - kernel);
					cv::Point tx(c, r); //(x, y)
					t_cur_y = tangent_coordinate.at<cv::Vec2d>(ty);
					double H = 1.0;
					double fai = myPhi(t_cur_x, t_cur_y);
					double Ws = 1;// myWs(tx, ty, kernel);
					double Wm = myWm(gradient_amplitude.at<double>(ty), gradient_amplitude.at<double>(tx), H);
					double Wd = myWd(t_cur_x, t_cur_y);
					t_new_x += fai * Ws * Wm * Wd * t_cur_y;
					sum += fai * Ws * Wm * Wd;
				}
			}
			if (fabs(sum) > 0.00001)
			{
				t_new_x = t_new_x / sum;
			}
			else
				t_new_x = cv::Vec2d(0.0, 0.0);
			tangent_new.at<cv::Vec2d>(r, c) = t_new_x;


		}
	}
}

void MagNormalize(Matrix1d &Mag)
{
	double max = 0.0;
	for (auto r = 0; r < Mag.rows; r++)
	{
		for (auto c = 0; c < Mag.cols; c++)
		{
			double val = Mag.at<double>(r, c);
			if (val > max)
			{
				max = val;
			}
		}
	}

	for (auto r = 0; r < Mag.rows; r++)
	{
		for (auto c = 0; c < Mag.cols; c++)
		{
			double v = Mag.at<double>(r, c);
			Mag.at<double>(r, c) = v / max;
		//	std::cout << Mag.at<double>(r, c) << std::endl;
		}
	}
}


void MagNormalize_uchar(Matrix1d &Mag)
{
	for (auto r = 0; r < Mag.rows; r++)
	{
		for (auto c = 0; c < Mag.cols; c++)
		{
			double v = Mag.at<double>(r, c);
			Mag.at<double>(r, c) = v * 255.0;
		}
	}
}

void VectorNormalize(cv::Mat &vf)
{
	for (auto r = 0; r < vf.rows; r++)
	{
		for (auto c = 0; c < vf.cols; c++)
		{
			cv::Vec2d v = vf.at<cv::Vec2d>(r, c);
			double mag_v = sqrt(v[0] * v[0] + v[1] * v[1]);
			if (abs(mag_v) < 0.00001)
			{
				continue;
			}
			vf.at<cv::Vec2d>(r, c) = v / mag_v;
		}
	}
}

void saveImage_Matrix1d(const Matrix1d &mat,const std::string &filename)
{
	cv::Mat Out(mat.rows, mat.cols, CV_64FC1);
	for (auto r = 0; r < mat.rows; r++)
	{
		for (auto c = 0; c < mat.cols; c++)
		{
			Out.at<double>(r, c) = mat.at<double>(r, c) * 255.0;
		}
	}
	imwrite("filename", Out);
}

void ETF(const cv::Mat &gray_img_In, cv::Mat &tangent_new, cv::Mat &gradient_coordinate, cv::Mat &tangent_coordinate)
{
	cv::Mat gradient_x(gray_img_In.rows, gray_img_In.cols, CV_64FC1);
	cv::Mat gradient_y(gray_img_In.rows, gray_img_In.cols, CV_64FC1);

	int wh = sqrt(sizeof(mysoble_template_x) / sizeof(mysoble_template_x[0]));
	cv::Mat template_kernel_x(wh, wh, CV_64FC1, cv::Scalar(0));
	convolution_kernel(mysoble_template_x, wh*wh, template_kernel_x);

	wh = sqrt(sizeof(mysoble_template_y) / sizeof(mysoble_template_y[0]));
	cv::Mat template_kernel_y(wh, wh, CV_64FC1, cv::Scalar(0));
	convolution_kernel(mysoble_template_y, wh*wh, template_kernel_y);

	Convolution_d(gray_img_In, template_kernel_x, gradient_x);
	Convolution_d(gray_img_In, template_kernel_y, gradient_y);
	cv::imwrite("./result/gradient_x.png", gradient_x);
	cv::imwrite("./result/gradient_y.png", gradient_y);

	Matrix1d gradient_amplitude(gray_img_In.rows, gray_img_In.cols, CV_64FC1, cv::Scalar(0));
	
	for (auto r = 1; r < gray_img_In.rows - 1; r++)
	{
		for (auto c = 1; c < gray_img_In.cols - 1; c++)
		{
			gradient_coordinate.at<cv::Vec2d>(r, c) = (gradient_x.at<double>(r, c), gradient_y.at<double>(r, c));
			tangent_coordinate.at<cv::Vec2d>(r, c) = cv::Vec2d(-gradient_y.at<double>(r, c), gradient_x.at<double>(r, c));
		}
	}
	CalculateMag(gradient_coordinate, gradient_amplitude);
	MagNormalize(gradient_amplitude);

	VectorNormalize(tangent_coordinate);  
	compute_TNewX(tangent_coordinate, gradient_amplitude, 2, tangent_new);
	write_file(tangent_new, "./result/TNewX.vecT");
}

void read_file(std::string filename, cv::Mat &source_img)
{
	std::ifstream input_file(filename);
	
	int img_rows, img_cols;
	input_file >> img_cols >> img_rows;

	cv::Mat img(img_rows, img_cols, CV_64FC2);
	for (auto r = 0; r < img_rows; r++)
	{
		for (auto c = 0; c < img_cols; c++)
		{
			cv::Vec2d v;
			input_file >> v[0] >> v[1];
			img.at<cv::Vec2d>(r, c) = v;
		}
	}
 	source_img = img;
}

void Compute_difference(const cv::Mat &Gos_sigmac, const cv::Mat &Gos_sigmas, double rou, cv::Mat &Gos_difference)
{
	for (auto r = 0; r < Gos_sigmac.rows; r++)
	{
		for (auto c = 0; c < Gos_sigmac.cols; c++)
		{
			double v1 = Gos_sigmac.at<double>(r, c);
			double v2 = Gos_sigmas.at<double>(r, c);
			Gos_difference.at<double>(r, c) = v1 - rou * v2;
			//std::cout <<r<<","<<c<<":"<< Gos_difference.at<double>(r, c) << std::endl;
		}
	}
}

int min(int a, int b)
{
	return a > b ? b : a;
}

void UcharToDouble(const cv::Mat In, cv::Mat Out)
{
	for (auto r = 0; r < In.rows; r++)
	{
		for (auto c = 0; c < In.cols; c++)
		{
			Out.at<double>(r,c) = In.at<uchar>(r, c)*1.0;

		}
	}
}

double bilinear_result(const cv::Mat &In, cv::Vec2i p1, cv::Vec2i p2, cv::Vec2i p3, cv::Vec2i p4, cv::Vec2d src_point)
{
	double aa = In.at<double>(p2[0],p2[1])*(p3[0] - src_point[0])*(src_point[1] - p3[1]);
	double bb = In.at<double>(p3[0],p3[1])*(src_point[0] - p2[0])*(p2[1] - src_point[1]);
	double cc = In.at<double>(p4[0],p4[1])*(src_point[0] - p1[0])*(src_point[1] - p1[1]);
	double dd = In.at<double>(p1[0],p1[1])*(p4[0] - src_point[0])*(p4[1] - src_point[1]);
	double result = aa + bb + cc + dd;
	return result;
}



//p取点由左至右由上至下
double bilinear_location(const cv::Mat &In, cv::Vec2d src_point)
{
	double result = 0.0;
	cv::Vec2i p1;/*(0,0)*/
	p1[0] = floor(src_point[0]);
	p1[1] = floor(src_point[1]);
	cv::Vec2i p2;/*(1,0)*/
	p2[0] = min(p1[0]+1,In.rows-1);
	p2[1] = p1[1];
	cv::Vec2i p3;/*(0,1)*/
	p3[0] = p1[0];
	p3[1] = min(p1[1]+1,In.cols-1);
	cv::Vec2i p4;/*(1,1)*/
	p4[0] = min(p1[0]+1, In.rows - 1);
	p4[1] = min(p1[1]+1, In.cols - 1);
	result = bilinear_result(In, p1, p2, p3, p4, src_point);
	//std::cout << "result="<<result << std::endl;
	return result;
}

void calculate_F(const cv::Mat &gradient_Tnew, const cv::Mat &gray_double, const cv::Mat &Gos_difference, cv::Mat &img_F)
{
	double det = 0.5;
	double T = 2.0;
	for (auto r = 0; r < gradient_Tnew.rows; r++)//281
	{
		for (auto c = 0; c < gradient_Tnew.cols; c++)//301
		{
			cv::Vec2d g = gradient_Tnew.at<cv::Vec2d>(r, c);
			double sum = 0.0;
			for (auto times = 1; times <= ceil(T / det); times++)
			{
				cv::Vec2d v_left( r - g[0] * det * times, c - g[1] * det * times);
				cv::Vec2d v_right( r + g[0] * det * times, c + g[1] * det * times);
				if (v_left[0] < 0 || v_left[0]>gradient_Tnew.rows - 1 || v_right[0] < 0 || v_right[0]>gradient_Tnew.rows - 1 || v_left[1] < 0 || v_left[1]>gradient_Tnew.cols - 1 || v_right[1] < 0 || v_right[1]>gradient_Tnew.cols - 1)
				{
					continue;
				}
				else
				{
					double v1 = bilinear_location(gray_double, v_left);
					double v2 = bilinear_location(Gos_difference, v_left);
					double v3 = bilinear_location(gray_double, v_right);
					double v4 = bilinear_location(Gos_difference, v_right);
					sum += v1 * v2 + v3 * v4;
					//std::cout << r<<","<< c << ":" << sum << std::endl;
				}
			}
			img_F.at<double>(r, c) = sum;
			//std::cout <<sum << std::endl;
		}
	}
}

void compute_gradient(const cv::Mat In, cv::Mat Out)
{
	for (auto r = 0; r < In.rows; r++)
	{
		for (auto c = 0; c < In.cols; c++)
		{
			cv::Vec2d val;
			val = In.at<cv::Vec2d>(r, c);
			Out.at<cv::Vec2d>(r, c) = (-val[1], val[0]);
		//	std::cout << r << "," << c << ":" << val[1] << "," << val[0] << std::endl;
		}
	}
}

void calculate_H(const cv::Mat &tangent_new, const cv::Mat &img_F, const cv::Mat &template_gos, cv::Mat &img_H)
{
	double det = 0.5;
	double T = 2.0;
	double kernel_template = (template_gos.rows - 1) / 2;

	for (auto r = 0; r < tangent_new.rows; r++)
	{
		for (auto c = 0; c < tangent_new.cols; c++)
		{
			double sum = template_gos.at<double>(kernel_template, kernel_template)*img_F.at<double>(r,c);
			cv::Vec2d t= tangent_new.at<cv::Vec2d>(r,c);
			cv::Vec2d mid_left(0,0);
			cv::Vec2d mid_right(0,0);
			cv::Vec2d step;
			cv::Vec2d v_left;
			cv::Vec2d v_right;
			for (auto times = 1; times <= ceil(T/det); times++)
			{
				double v1=0, v2=0, v3=0, v4=0;
				if (times != 1)
				{
					step = t;
					v_left = mid_left - step;
					v_right = mid_right + step;
				}
				else
				{
					step = t;
					v_left[0] =  r - step[0];
					v_left[1] = c - step[1];
					v_right[0] = r + step[0];
					v_right[1] = c + step[1];
				}
				mid_left = v_left;
				mid_right = v_right;

				if (v_left[0] < 0 || v_left[0]>img_F.rows - 1 ||
					v_right[0] < 0 || v_right[0]>img_F.rows - 1 ||
					v_left[1] < 0 || v_left[1]>img_F.cols - 1 ||
					v_right[1] < 0 ||v_right[1]>img_F.cols - 1)
				{
					continue;
				}
				else
				{
					v2 = bilinear_location(img_F, v_left);
					v4 = bilinear_location(img_F, v_right);
				}
				if (abs(t[0])> kernel_template|| abs(t[1]) > kernel_template)
				{
					continue;
				}
				else
				{
					cv::Vec2d tnew(t[0] + kernel_template, t[1] + kernel_template);
					v1 = bilinear_location(template_gos, tnew);
					v3 = bilinear_location(template_gos, tnew);
				}
				t[0] = bilinear_location(tangent_new, v_left);
				t[1] = bilinear_location(tangent_new, v_right);
				sum += v1 * v2 + v3 * v4;
			}
			//std::cout << sum << std::endl;
			img_H.at<double>(r, c) = sum;
		}
	}
}

void binaryzation(cv::Mat &img_H, cv::Mat &binaryzation_img)
{
	double tao = 0.5;
	for (auto r = 0; r < img_H.rows; r++)
	{
		for (auto c = 0; c < img_H.cols; c++)
		{
			double val = img_H.at<double>(r, c);
			if (val < 0 || 1 + tanh(val) < tao)
			{
				img_H.at<double>(r, c) = 0.0;
			}
			else
			{
				img_H.at<double>(r, c) = 1.0;
			}
		}
	}
}


int main()
{
	std::string source_img_name = "./data/eagle2.jpg";
	cv::Mat source_img = cv::imread(source_img_name,1);
	cv::Mat gray_img;
	cv::cvtColor(source_img, gray_img, cv::COLOR_BGR2GRAY);
	cv::Mat gray_double(gray_img.rows, gray_img.cols, CV_64FC1);
	UcharToDouble(gray_img, gray_double);

	//求ETF
  	cv::Mat tangent_line = source_img.clone();
 	cv::Mat tangent_img(gray_img.rows, gray_img.cols,CV_64FC2);
 	cv::Mat tangent_new(gray_img.rows, gray_img.cols, CV_64FC2, cv::Scalar(0, 0));
 	Matrix2d gradient_coordinate(gray_img.rows, gray_img.cols, CV_64FC2, cv::Scalar(0, 0));
 	Matrix2d tangent_coordinate(gray_img.rows, gray_img.cols, CV_64FC2, cv::Scalar(0, 0));
 	ETF(gray_double, tangent_new, gradient_coordinate, tangent_coordinate);
 	read_file("./result/TNewX.vecT", tangent_new);
	VectorNormalize(tangent_new);

	//求高斯差
	double sigma_s;
	double sigma_c=1.0;
	sigma_s = 1.6*sigma_c;
	double rou = 0.99;
	cv::Mat sigma_c_Gos(gray_img.rows, gray_img.cols, CV_64FC1);
	cv::Mat template_c;
	template_gos(5, sigma_c, template_c);
	Convolution_d(gray_double, template_c, sigma_c_Gos);
	imwrite("./result/sigma_c_Gos.png", sigma_c_Gos);
	cv::Mat sigma_s_Gos(gray_img.rows, gray_img.cols, CV_64FC1);
	cv::Mat template_s;
	template_gos(5, sigma_s, template_s);
	Convolution_d(gray_double, template_s, sigma_s_Gos);
	imwrite("./result/sigma_s_Gos.png", sigma_s_Gos);
 	cv::Mat Gos_difference(gray_img.rows, gray_img.cols, CV_64FC1, cv::Scalar(0));
  	Compute_difference(sigma_c_Gos, sigma_s_Gos, rou, Gos_difference);
//   	MagNormalize(Gos_difference);
//  	MagNormalize_uchar(Gos_difference);
//  	cv::imwrite("./result/Gos_difference.png", Gos_difference);	
 	cv::Mat gradient_Tnew(tangent_new.rows, tangent_new.cols, CV_64FC2);
 	compute_gradient(tangent_coordinate, gradient_coordinate);
  	cv::Mat img_F(gray_img.rows, gray_img.cols, CV_64FC1, cv::Scalar(0));
  	calculate_F(gradient_coordinate, gray_double, Gos_difference, img_F);
 	cv::imwrite("./result/overstriking_source5.png", img_F);

	double sigma_m = 1.0;
	cv::Mat template_m;
	template_gos(5, sigma_m, template_m);
	cv::Mat img_H(gray_img.rows, gray_img.cols, CV_64FC1, cv::Scalar(0));
	calculate_H(tangent_coordinate, img_F, template_m, img_H);
	cv::imwrite("./result/lengthen_source5.png", img_H);
	MagNormalize(img_H);
	cv::imwrite("./result/lengthen_normal.png", img_H);
	cv::Mat binaryzation_img(gray_img.rows, gray_img.cols, CV_64FC1, cv::Scalar(0));
	binaryzation(img_H, binaryzation_img);
	cv::imwrite("./result/binaryzation_img.png", binaryzation_img);
	std::system("pause");
	cv::waitKey(0);
}


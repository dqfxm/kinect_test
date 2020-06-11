//transformation_depth_image_to_color_camera.cpp//
//c风格代码将相机捕捉的深度图像转换到彩色相机几何视角，代码已测试可用//
#pragma comment(lib, "k4a.lib")
#include <stdio.h>
#include <k4a/k4a.h>
#include <k4arecord/record.h>
#include <k4arecord/playback.h>
#include <iostream>
#include <stdlib.h>

#include <k4a/k4a.hpp>
#include <cstdlib>

#include <ctime>
#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <math.h>
#include <fstream>
#include <sstream>


//using namespace cv;
using namespace std;
clock_t time_start, time_end; //定义时间类型

//以下定义一个把深度图像转换成彩色相机视角的静态函数，返回值为bool类型
static bool transformation_depth_to_color(k4a_transformation_t transformation_handle,
	const k4a_image_t depth_image,
	const k4a_image_t color_image)
{
	//transform color image into depth camer
	int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
	int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
	k4a_image_t transformed_depth_image = NULL;
	if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
												color_image_width_pixels,
												color_image_height_pixels,
												color_image_width_pixels * (int)sizeof(uint16_t),
												&transformed_depth_image))
	{
		printf("创建深度图像失败|Failed to create transformed depth image\n");
		return false;
	}
	if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_color_camera(transformation_handle,
		depth_image, transformed_depth_image))
	{
		printf("计算转换后深度图像失败|Failed to compute transformed depth image\n");
		return false;
	}
	k4a_image_release(transformed_depth_image);

	return true;
}

int main()
{
	uint8_t deviceID = K4A_DEVICE_DEFAULT;
	k4a_device_t device = NULL;
	const int32_t TIMEOUT_IN_MS = 10000;
	k4a_transformation_t transformation = NULL;
	k4a_capture_t capture = NULL;
	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL; //初始化配置参数
	k4a_image_t depth_image = NULL;
	k4a_image_t color_image = NULL;

	uint32_t device_count = 0;

	device_count = k4a_device_get_installed_count();//查看设备个数

	if (device_count == 0)
	{
		printf("No K4A devices found\n");
		return 0;
	}
	if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceID, &device))
	{
		printf("Failed to open device\n");
		goto Exit;
	}
	//初始化相机配置参数
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;
	config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
	config.camera_fps = K4A_FRAMES_PER_SECOND_30;
	config.synchronized_images_only = true; //ensures that depth and color images are both available in the capture

	//设备校准
	k4a_calibration_t calibration;
	if (K4A_RESULT_SUCCEEDED != k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
	{
		printf("Failed to get calibration\n");
		goto Exit;
	}
	printf("Calibration succeed!\n");

	//创建图像转换句柄
	transformation = k4a_transformation_create(&calibration);
	
	//按配置打开相机
	if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
	{ 
		printf("Failed to start cameras\n");
		goto Exit;
	}
	
	//Get a capture 图像捕捉
	switch (k4a_device_get_capture(device,&capture,TIMEOUT_IN_MS))
	{
	case K4A_WAIT_RESULT_SUCCEEDED:
		break;
	case K4A_WAIT_RESULT_TIMEOUT:
		printf("Timed out waiting for a capture\n");
		goto Exit;
	case K4A_WAIT_RESULT_FAILED:
		printf("Failed to read a capture\n");
		goto Exit;
	}

	//Get a depth image
	depth_image = k4a_capture_get_depth_image(capture);
	if (depth_image == 0)
	{
		printf("Failed to get depth image from capture\n");
		goto Exit;
	}

	//Get a color image
	color_image = k4a_capture_get_color_image(capture);
	if (color_image == 0)
	{
		printf("Failed to get color image from capture\n");
		goto Exit;
	}

	//调用函数，转换深度到彩色相机几何，Warping depth image into color camera geometry
	if (transformation_depth_to_color(transformation, depth_image, color_image) == false)
	{
		goto Exit;
	}
	printf("成功将深度图像转换到彩色相机几何！\n");

	//定义程序退出条件
Exit:
	if (depth_image != NULL)
	{
		k4a_image_release(depth_image);
	}
	if (color_image != NULL)
	{
		k4a_image_release(color_image);
	}
	if (capture != NULL)
	{
		k4a_capture_release(capture);
	}
	if (transformation != NULL)
	{
		k4a_transformation_destroy(transformation);
	}
	if (device != NULL)
	{
		k4a_device_stop_cameras(device);
		k4a_device_close(device);
	}
	return 0;
}

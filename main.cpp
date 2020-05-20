/* Copyright (c) 2020 TaegyunHa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

// OpenCv Library
#include <opencv2/opencv.hpp>

// Standard Libaray
#include <atomic>
#include <condition_variable>
#include <execution>
#include <iostream>
#include <vector>
#include <memory>

#include "CvImshow.h"

// Application Props
static constexpr int nEXAMPLE = 3;

/* This exmaple will show you basic implementation
 * of <CvImShowThread>*/
void test0()
{
	// Create 300x300 blue image
	cv::Mat sampleImg(300, 300, CV_8UC3);
	sampleImg.setTo(cv::Scalar(255, 0, 0));

	// 1. Simple imshow implementation ------------
	CvImShowThread imshowObj("test0: basic Impl1");
	imshowObj.imshow(sampleImg);
	
	std::cout << "Press Enter to hide the displayed window\n";
	std::cin.get();
	imshowObj.setDisplay(false);	// hide window

	std::cout << "Press Enter to call <setDisplay(true)>\n";
	std::cin.get();
	imshowObj.setDisplay(true);
	std::cout << "The window has not been displayed yet since there was no new image\n"
		<< "the window will be displayed as soon as <imshow(...)> is called with a new image\n";
	std::cin.get();
	imshowObj.imshow(sampleImg);

	std::cout << "Press Enter to hide the window again\n";
	std::cin.get();
	imshowObj.setDisplay(false);
	imshowObj.imshow(sampleImg);
	std::cout << "Although a new image has been passed to imShow object by calling <imshow(...)>,\n"
		<< "you can see that the window is still hidden due to <setDisplay(false)>.\n";
	std::cin.get();

	imshowObj.setDisplay(true);
	std::cout << "Although <imshow(...)> has not been called after <setDisplay(true), \n"
		<< "you can see that the window has displayed. This is becuase, <imshow(...)> had been called,\n"
		<< "already after previous image was displayed.\n";
	std::cin.get();

	// Finish Simple imshow implementation 
	imshowObj.setDisplay(false);


	// 2. Set the imshow props -------------------
	CvImShowThread imshowObj2("test0: basic Impl2");
	imshowObj2.setWindowLocation(300, 300);
	imshowObj2.imshow(sampleImg);
	std::cout << "you can set the location of the window before display it\n";
	std::cin.get();

	imshowObj2.moveWindow(400, 400);
	std::cout << "you can also move the window as cv::moveWindow(...) does\n";
	std::cin.get();
}

/* This will generate <nThread> number of threads
 * in parallel unsequnced order to display multiple
 * test images.
 * You can set:
 * 	- windowCols  : the number of windows in column
 * 	- windowWidth : a width of test image
 * 	- windowHeight: a height of test image */
void test1(int nThread)
{
	static constexpr int windowCols = 4;
	static constexpr int windowWidth = 200;
	static constexpr int windowHeight = 200;

	std::atomic<bool> isAlive(true);
	std::vector<std::thread> threads(nThread);
	std::atomic<int> threadIdx(0);

	std::for_each(
		std::execution::par_unseq,
		threads.begin(),
		threads.end(),
		[&](std::thread& rThread)
	{
		rThread = std::thread([&](int threadIdx)
		{
			cv::Mat testImg(windowWidth, windowHeight, CV_8UC3);
			std::string windowName = "test1: MultiThread Imshow" + std::to_string(threadIdx);

			int row = threadIdx / windowCols;
			int rowInPx = row * windowHeight;
			int col = threadIdx % windowCols;
			int colInPx = col * windowWidth;

			CvImShowThread imShowThread(windowName);
			imShowThread.setWindowLocation(colInPx, rowInPx);
			imShowThread.setWindowFlags(cv::WindowFlags::WINDOW_AUTOSIZE);
			
			uint8_t testColorVal = 0;
			while (isAlive.load())
			{
				testImg.setTo(cv::Scalar(testColorVal++, 0, 0));
				imShowThread.imshow(testImg);

				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}, threadIdx.fetch_add(1));
	});

	// Wait user key input to finish the test
	std::cout << "Press Enter To Finish Test 1\n";
	std::cin.get();
	isAlive.store(false);

	// Threads Clean process
	for (auto& rThread : threads)
	{
		if(rThread.joinable())
			rThread.join();
	}
}

/* This will generate the one main thread that displays image
 * by conventional opencv <cv::imshow> with multiple <CvImShowThread>.
 * With this test, you can see that moving main window blocks all the 
 * other sub windows; however, moving sub window does not block any of
 * other windows */
void test2(int nSubWindows)
{
	static constexpr int windowCols = 4;
	static constexpr int windowWidth = 200;
	static constexpr int windowHeight = 200;
	static constexpr int mainWindowWidth = 600;
	static constexpr int mainWindowHeight = 600;

	std::atomic<bool> isAlive(true);
	std::thread thread([&]()
	{
		// Construct <nSubWindows> number of <CvImShowThread>
		std::vector<std::unique_ptr<CvImShowThread>> imShowThreads;
		for (int i = 0; i < nSubWindows; ++i)
		{
			std::string subWindowName = "test2: subWindow " + std::to_string(i);
			int row = i / windowCols;
			int rowInPx = row * windowHeight;
			int col = i % windowCols;
			int colInPx = col * windowWidth;

			imShowThreads.emplace_back(std::make_unique<CvImShowThread>(subWindowName));
			imShowThreads.back()->setWindowLocation(colInPx, rowInPx);
			imShowThreads.back()->setWindowFlags(cv::WindowFlags::WINDOW_AUTOSIZE);
		}

		// Create main window
		cv::namedWindow("test2: MainWindow");
		cv::moveWindow("test2: MainWindow", windowWidth * windowCols, 0);

		// Main process loop
		cv::Mat subImg(windowWidth, windowHeight, CV_8UC3);
		cv::Mat mainImg(mainWindowWidth, mainWindowHeight, CV_8UC3);
		uint8_t testColorVal = 0;
		while (isAlive.load())
		{
			mainImg.setTo(cv::Scalar(0, testColorVal++, 0));
			subImg.setTo(cv::Scalar(0, testColorVal++, 0));

			std::for_each(
				std::execution::par_unseq,
				imShowThreads.begin(),
				imShowThreads.end(),
				[&](std::unique_ptr<CvImShowThread>& rpImShowThread)
			{
				rpImShowThread->imshow(subImg.clone());
			});

			cv::imshow("test2: MainWindow", mainImg);
			cv::waitKey(10);
		}
	});

	// Wait user key input to finish the test
	std::cout << "Press Enter To Finish Test 2\n";
	std::cin.get();
	isAlive.store(false);

	if (thread.joinable())
		thread.join();
}

int main()
{
	test0();

	int nThreads = 10;
	test1(nThreads);

	int nSubWindows = 6;
	test2(nSubWindows);

	return 0;
}

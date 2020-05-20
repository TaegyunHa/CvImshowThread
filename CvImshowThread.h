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

#pragma once
#include <opencv2/opencv.hpp>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <thread>
#include <mutex>

namespace CvImshowHelpFn
{
	class ICvImshowFn
	{
	public:
		virtual void impl(const std::string& windowName) = 0;
	};

	class SetWindowProperty : public ICvImshowFn
	{
	public:
		SetWindowProperty(const cv::WindowPropertyFlags propertyFlags, const double propValue)
			:m_propertyFlags(propertyFlags), m_propValue(propValue) {}

		void impl(const std::string& windowName) override
		{
			cv::setWindowProperty(windowName, m_propertyFlags, m_propValue);
		}
	private:
		const cv::WindowPropertyFlags m_propertyFlags;
		const double m_propValue;
	};

	class SetWindowTitle : public ICvImshowFn
	{
	public:
		SetWindowTitle(const std::string& title)
			:m_title(title) {}

		void impl(const std::string& windowName) override
		{
			cv::setWindowTitle(windowName, m_title);
		}
	private:
		const std::string m_title;
	};

	class ResizeWindowWH : public ICvImshowFn
	{
	public:
		ResizeWindowWH(const int width, const int height)
			:m_width(width), m_height(height) {}

		void impl(const std::string& windowName) override
		{
			cv::resizeWindow(windowName, m_width, m_height);
		}
	private:
		const int m_width;
		const int m_height;
	};

	class ResizeWindowSz : public ICvImshowFn
	{
	public:
		ResizeWindowSz(const cv::Size& size)
			:m_size(size) {}

		void impl(const std::string& windowName) override
		{
			cv::resizeWindow(windowName, m_size);
		}
	private:
		const cv::Size m_size;
	};

	class MoveWindow : public ICvImshowFn
	{
	public:
		MoveWindow(const int x, const int y)
			:m_x(x), m_y(y) {}

		void impl(const std::string& windowName) override
		{
			cv::moveWindow(windowName, m_x, m_y);
		}
	private:
		const int m_x;
		const int m_y;
	};
};

class CvImShowThread
{
public:
	CvImShowThread(const std::string& windowName)
		: m_windowName(windowName), m_isAlive(true),
		m_isWindowDistroyed(true), m_isNewImg(false), m_display(true),
		m_waitKey(1), m_windowFlags(cv::WINDOW_NORMAL)
	{
		m_processThread = std::thread([this]()
		{
			while (m_isAlive.load())
			{
				std::unique_lock<std::mutex> ulock(m_imgMtx);
				m_newImgCv.wait(ulock, [&]
				{
					if (!m_display.load())
					{
						std::lock_guard<std::mutex> guard(m_windowMtx);
						if (!m_isWindowDistroyed)
						{
							cv::destroyWindow(m_windowName);
							m_isWindowDistroyed = true;
						}
					}

					return (!m_isAlive.load()
						|| m_isNewImg && m_display.load()	// new image
						|| m_pImshowFnBuffer.size() > 0);	// new function call
				});

				if (!m_isAlive.load())
					break;

				// A new image to show exists
				if (m_display.load() && m_isNewImg)
				{
					cv::Mat dispImg;
					if (m_isNewImg)
						cv::swap(dispImg, m_newImg);
					m_isNewImg = false;
					ulock.unlock();

					if (!dispImg.empty())
					{
						std::lock_guard<std::mutex> guard(m_windowMtx);
						if (m_isWindowDistroyed)
						{
							cv::namedWindow(m_windowName, m_windowFlags);
							cv::moveWindow(m_windowName, m_x, m_y);
							m_isWindowDistroyed = false;
						}
						cv::imshow(m_windowName, dispImg);
						cv::waitKey(m_waitKey.load());
					}
				}

				// Imshow function buffer to implement exists
				while (m_pImshowFnBuffer.size() > 0)
				{
					m_pImshowFnBuffer.front()->impl(m_windowName);
					m_pImshowFnBuffer.pop_front();
				}
			}
		});
	}

	~CvImShowThread()
	{
		m_isAlive.store(false);
		m_newImgCv.notify_all();
		if (m_processThread.joinable())
			m_processThread.join();

		cv::destroyWindow(m_windowName);
	}

	void imshow(const cv::Mat& newImg)
	{
		std::unique_lock<std::mutex> ulock(m_imgMtx);
		m_newImg = newImg.clone();
		m_isNewImg = true;
		ulock.unlock();
		m_newImgCv.notify_one();
	}

	void setDisplay(const bool display)
	{
		m_display = display;
		m_newImgCv.notify_one();
	}

	void setWindowName(const std::string& newWindowName)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		if (!m_isWindowDistroyed)
		{
			cv::destroyWindow(m_windowName);
			m_isWindowDistroyed = true;
		}
		m_windowName = newWindowName;
	}

	// Set imshow props
	inline void setWaitKey(int waitTime) { m_waitKey.store(waitTime); }
	inline void setWindowFlags(cv::WindowFlags windowFlags) { m_windowFlags = windowFlags; }
	inline void setWindowLocation(const int x, const int y)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_x = x;
		m_y = y;
		if (!m_isWindowDistroyed)
			moveWindow(x, y);
	}
	void setWindowProperty(const cv::WindowPropertyFlags propertyFlags, const double propValue)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_pImshowFnBuffer.emplace_back(std::make_unique<CvImshowHelpFn::SetWindowProperty>(propertyFlags, propValue));
		m_newImgCv.notify_one();
	}
	void setWindowTitle(const std::string& title)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_pImshowFnBuffer.emplace_back(std::make_unique<CvImshowHelpFn::SetWindowTitle>(title));
		m_newImgCv.notify_one();
	}
	void resizeWindow(int width, int height)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_pImshowFnBuffer.emplace_back(std::make_unique<CvImshowHelpFn::ResizeWindowWH>(width, height));
		m_newImgCv.notify_one();
	}
	void resizeWindow(const cv::Size& size)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_pImshowFnBuffer.emplace_back(std::make_unique<CvImshowHelpFn::ResizeWindowSz>(size));
		m_newImgCv.notify_one();
	}
	void moveWindow(int x, int y)
	{
		std::lock_guard<std::mutex> guard(m_windowMtx);
		m_pImshowFnBuffer.emplace_back(std::make_unique<CvImshowHelpFn::MoveWindow>(x, y));
		m_newImgCv.notify_one();
	}
	
	//Delete the Assignment opeartor
	CvImShowThread& operator=(const CvImShowThread&) = delete;

private:
	// Window Prop
	std::string m_windowName;
	std::thread m_processThread;
	std::atomic<bool> m_isAlive;
	std::mutex m_windowMtx;
	bool m_isWindowDistroyed;

	// Img Props
	cv::Mat m_newImg;
	std::mutex m_imgMtx;
	std::condition_variable m_newImgCv;
	std::atomic<bool> m_display;
	bool m_isNewImg;

	// imshow Props
	std::atomic<int> m_waitKey;
	cv::WindowFlags m_windowFlags;
	int m_x = 0;
	int m_y = 0;

	// inshow function buffer
	std::deque<std::unique_ptr<CvImshowHelpFn::ICvImshowFn>> m_pImshowFnBuffer;
};

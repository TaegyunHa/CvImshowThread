# CvImshowThread
CvImShowThread is a header only class that manages the opencv cv::imshow from the seperate thread.\n
This will allow displaying opencv imshow from multiple threads.

## What it does
**CvImshowThread** will generate seperate thread and manage all <cv::imshow> properties within the thread.\
One of drawbacks of ``cv::imshow`` is that, setting a window properties such as `cv::moveWindow(...)` or `cv::setWindowProperty(...)` cannot be called from different thread. **CvImshowThread** is hereby allowing user to have multiple ``cv::imshow`` from multiple threads as well as set property from a different thread.

## Limitation
* As ``cv::namedWindow(...)`` is the backend of this wrapper class, same window name cannot be used by two different object.
* Currently it does not support ```cv::getWindowProperty(...)``` or any other getter from openCv
* Calling conventional OpenCv function such as ```cv::setWindowProperty(...)``` with same window name of **CvImshowThread** will freeze the displayed window as OpenCv does.
  * Instead you can call the function by ``CvImshowThread.setWindowProperty(...)``
  
## Added features

## License
CvImshowThread is under MIT License:

Copyright (c) 2020 TaegyunHa

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
SOFTWARE.

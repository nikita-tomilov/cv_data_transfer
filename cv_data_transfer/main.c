#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"
#include "image_filters.h"

int main(int argc, char* argv[])
{
	/* initialising camera */
	CvCapture* capture = cvCreateCameraCapture(CV_CAP_ANY); 
	assert(capture);

	/* initialising window */
	cvNamedWindow("capture", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("settings", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("filtered", CV_WINDOW_AUTOSIZE);

	/* initialising images */
	IplImage* frame = cvQueryFrame(capture); /* webcam image */
	IplImage* dst = 0;  /*destination for all magic */
	IplImage* bw = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1); /* bw image used for finding circles */
	IplImage* hls = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels); /* image in hls for changing saturation */
	IplImage* filtered = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels); /* image after filtering */
	
	/* adjusting trackbars, though they change nothing right now */

	int sat = 150;
	cvCreateTrackbar("Saturation", "settings", &sat, 400, NULL);

	int bin_border = 150;
	cvCreateTrackbar("Treshold", "settings", &bin_border, 255, NULL);

	int b_treshold[2];
	b_treshold[0] = 0;
	b_treshold[1] = 255;
	cvCreateTrackbar("B Treshold Min", "settings", &b_treshold[0], 255, NULL);
	cvCreateTrackbar("B Treshold Max", "settings", &b_treshold[1], 255, NULL);

	while (1)
	{
		/* retrieving image */
		frame = cvQueryFrame(capture);
		dst = cvClone(frame);

		
		/*cvAddWeighted(frame, 1.5, dst, -0.5, 0, dst);
		

		cvCvtColor(frame, hls, CV_BGR2HLS);

		applyFuncOnImage(hls, hls, 1, &sat, calculateSaturationPixel);
		
		
		cvCvtColor(hls, dst, CV_HLS2BGR);

		applyFuncOnImage(dst, filtered, 2, b_treshold, calculateBTresholdPixel);
		applyFuncOnImage(dst, filtered, 1, &bin_border, calculateBrightnessTresholdPixel);
		*/
		cvCvtColor(dst, bw, CV_BGR2GRAY);
		cvSmooth(bw, bw, CV_GAUSSIAN, 5, 5, 3, 3);
		/*cvThreshold(bw, bw, bin_border, 255, CV_THRESH_BINARY);

		cvEqualizeHist(frame, dst);*/
		
		/* drawing all uniq circles */
		int n = 0;
		int n1 = 0;
		Circle_t* cir, cur;
		cir = getAllCirlces(bw, &n1);
		cir = getUniqCircles(cir, n1, 15, &n);
		if (n > 10) n = 10;

		for (int i = 0; i < n; i++)
		{
			cur = cir[i];
			cvCircle(dst, cvPoint(cur.x, cur.y), cur.r, cvScalar(0,0, 255,0), 1, 8, 0);
		}

		/* showing image */
		cvShowImage("capture", dst);
		cvShowImage("filtered", bw);

		char c = cvWaitKey(33);
		if (c == 27) { /* esc key pressed */
			return;
			
		}

	}
}


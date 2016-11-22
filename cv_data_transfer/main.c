#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"
#include "image_filters.h"

/* used to treshold on blue tracking points */
/* b, g, r, radius */
int g_tracking_values[] = { 0, 0, 0, 10, 10, 10 };

IplImage* frame;

void mouseCallback(int mevent, int x, int y, int flags, void* userdata)
{
	if (mevent == CV_EVENT_LBUTTONUP)
	{
		Pixel_t px;
		px = getPixel(frame, x, y);
		g_tracking_values[0] = px.r;
		g_tracking_values[1] = px.g;
		g_tracking_values[2] = px.b;
		/* printf("lmouse %d %d\n", x, y); */
	}
}

int main(int argc, char* argv[])
{
	/* initialising camera */
	CvCapture* capture = cvCreateCameraCapture(CV_CAP_ANY); 
	assert(capture);

	/* initialising window */
	cvNamedWindow("capture", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("settings", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("filtered", CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback("capture", mouseCallback, NULL);

	/* initialising images */
	frame = cvQueryFrame(capture); /* webcam image */
	IplImage* dst = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels);  /*destination for all magic */
	IplImage* bw = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1); /* bw image used for finding circles */
	IplImage* hls = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels); /* image in hls for changing saturation */
	IplImage* filtered = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels); /* image after filtering */
	
	CvFont main_font;

	/* adjusting trackbars */
	cvCreateTrackbar("R", "settings", g_tracking_values, 255, NULL);
	cvCreateTrackbar("G", "settings", g_tracking_values + 1, 255, NULL);
	cvCreateTrackbar("B", "settings", g_tracking_values + 2, 255, NULL);
	cvCreateTrackbar("Radius R", "settings", g_tracking_values + 3, 255, NULL);
	cvCreateTrackbar("Radius G", "settings", g_tracking_values + 4, 255, NULL);
	cvCreateTrackbar("Radius B", "settings", g_tracking_values + 5, 255, NULL);

	cvInitFont(&main_font, CV_FONT_HERSHEY_COMPLEX, 1.0f, 1.0f, 0, 1, CV_AA);


	while (1)
	{
		cvSetTrackbarPos("R", "settings", g_tracking_values[0]);
		cvSetTrackbarPos("G", "settings", g_tracking_values[1]);
		cvSetTrackbarPos("B", "settings", g_tracking_values[2]);


		/* retrieving image */
		frame = cvQueryFrame(capture);
		dst = cvClone(frame);

		applyFuncOnImage(frame, dst, 4, g_tracking_values, calculateTresholdByRGBValue);


		/*cvAddWeighted(frame, 1.5, dst, -0.5, 0, dst);


		cvCvtColor(frame, hls, CV_BGR2HLS);

		applyFuncOnImage(hls, hls, 1, &sat, calculateSaturationPixel);


		cvCvtColor(hls, dst, CV_HLS2BGR);

		applyFuncOnImage(dst, filtered, 2, b_treshold, calculateBTresholdPixel);
		applyFuncOnImage(dst, filtered, 1, &bin_border, calculateBrightnessTresholdPixel);
		*/
		cvCvtColor(dst, bw, CV_BGR2GRAY);
		cvSmooth(bw, bw, CV_GAUSSIAN, 15, 0, 3, 3);
		/*cvThreshold(bw, bw, bin_border, 255, CV_THRESH_BINARY);

		cvEqualizeHist(frame, dst);*/

		/* drawing all uniq circles */
		int n = 0;
		int n1 = 0;
		Circle_t* cir, cur;
		cir = getAllCirlces(bw, &n1);
		cir = getUniqCircles(cir, n1, 15, &n);

		if (n > 10) n = 10;

		if (n == 3)
		{
		
			CvPoint centers[3];
			cvPutText(frame, "Data Frame recognized.", cvPoint(30, 30),
				&main_font, cvScalar(0, 255, 0, 0));

			for (int i = 0; i < 3; i++)
			{
				cur = cir[i];
				cvCircle(frame, cvPoint(cur.x, cur.y), cur.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
				centers[i].x = cur.x;
				centers[i].y = cur.y;
			}

			cvLine(frame, centers[0], centers[1], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);
			cvLine(frame, centers[1], centers[2], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);
			cvLine(frame, centers[0], centers[2], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);
		}
		else
		{
			cvPutText(frame, "No Data Frame recognized.", cvPoint(30, 30),
				&main_font, cvScalar(0, 0,  255, 0));
		}

		

		/* showing image */
		cvShowImage("capture", frame);
		cvShowImage("filtered", bw);

		char c = cvWaitKey(33);
		if (c == 27) { /* esc key pressed */
			return;
			
		}

	}
}


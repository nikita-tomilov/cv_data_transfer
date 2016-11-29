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

int bits_recieved[8];
int bits_count = 0;
int byte_recieved = 0;
int previous_byte_recieved = 0;
int sync_counted = 0;


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
	if (mevent == CV_EVENT_RBUTTONUP)
	{
		printf("reset.\n");
		bits_count = 0;
		byte_recieved = 0;
		previous_byte_recieved = 0;
		sync_counted = 0;
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

	cvInitFont(&main_font, CV_FONT_HERSHEY_COMPLEX_SMALL, 1.0f, 1.0f, 0, 1, CV_AA);

	Circle_t old_circles[3];
	int found_data_frame = 0;
	Circle_t top_left;
	Circle_t top_right;
	Circle_t bottom_left;
	Circle_t sync, data;
	
	Circle_t* cir, cur;


	while (1)
	{
		cvSetTrackbarPos("R", "settings", g_tracking_values[0]);
		cvSetTrackbarPos("G", "settings", g_tracking_values[1]);
		cvSetTrackbarPos("B", "settings", g_tracking_values[2]);


		/* retrieving image */
		frame = cvQueryFrame(capture);
		dst = cvClone(frame);

		applyFuncOnImage(frame, dst, 4, g_tracking_values, calculateTresholdByRGBValue);


		cvCvtColor(dst, bw, CV_BGR2GRAY);
		cvSmooth(bw, bw, CV_GAUSSIAN, 15, 0, 3, 3);
		cvDilate(bw, bw, cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE, NULL), 2);
		
		/* drawing all uniq circles */
		int n = 0;
		int n1 = 0;
		
		cir = getAllCirlces(bw, &n1);
		cir = getUniqCircles(cir, n1, 15, &n);

		if (n > 10) n = 10;

		if (n == 3)
		{
			found_data_frame = 1;

			CvPoint centers[3];
			cvPutText(frame, "Data Frame recognized.", cvPoint(30, 30),
				&main_font, cvScalar(0, 255, 0, 0));

			calculateCircles(cir, &top_left, &top_right, &bottom_left, &sync, &data);

			for (int i = 0; i < 3; i++)
			{
				cur = cir[i];
				centers[i].x = cur.x;
				centers[i].y = cur.y;
				old_circles[i] = cir[i];
			}

			/* drawing markers */
			cvCircle(frame, cvPoint(top_left.x, top_left.y), top_left.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
			cvCircle(frame, cvPoint(top_right.x, top_right.y), top_right.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
			cvCircle(frame, cvPoint(bottom_left.x, bottom_left.y), bottom_left.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
			/* drawing sync and data */
			cvCircle(frame, cvPoint(data.x, data.y), data.r, cvScalar(255, 255, 255, 0), 1, 8, 0);
			cvCircle(frame, cvPoint(sync.x, sync.y), sync.r, cvScalar(255, 0, 0, 0), 1, 8, 0);
			/*drawing beautiful lines */
			/*cvLine(frame, centers[0], centers[1], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);
			cvLine(frame, centers[1], centers[2], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);
			cvLine(frame, centers[0], centers[2], cvScalar(0, 0, 255, 0), 1, CV_AA, 0);*/
		}
		else
		{
			cvPutText(frame, "No Data Frame recognized.", cvPoint(30, 30),
				&main_font, cvScalar(0, 0,  255, 0));
			if (found_data_frame)
			{
				/* drawing markers */
				cvCircle(frame, cvPoint(top_left.x, top_left.y), top_left.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
				cvCircle(frame, cvPoint(top_right.x, top_right.y), top_right.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
				cvCircle(frame, cvPoint(bottom_left.x, bottom_left.y), bottom_left.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
				/* drawing sync and data */
				cvCircle(frame, cvPoint(data.x, data.y), data.r, cvScalar(128, 128, 128, 0), 1, 8, 0);
				cvCircle(frame, cvPoint(sync.x, sync.y), sync.r, cvScalar(128, 128, 128, 0), 1, 8, 0);
			}
		}

		if (found_data_frame)
		{
			
			if (bitSet(frame, &sync, 230))
			{
				cvPutText(frame, "Sync bit is enabled.", cvPoint(30, 60),
					&main_font, cvScalar(255, 0, 0, 0));
				sync_counted++;
				
				if (sync_counted > 0)
				{
					printf("Sync found.\n");
					sync_counted == 0;
					if (bitSet(frame, &data, 230))
					{
						cvPutText(frame, "Data bit is enabled.", cvPoint(30, 90),
							&main_font, cvScalar(255, 255, 255, 0));
						bits_recieved[bits_count] = 1;
						printf(">Recieved 1.\n");
					}
					else
					{
						bits_recieved[bits_count] = 0;
						printf(">Recieved 0.\n");
					}
					//printf(">>%d\n", bits_recieved[bits_count]);
					bits_count++;
					if (bits_count = 8)
					{
						printf(">Recieved 8 bits.\n");
						bits_count = 0;
						byte_recieved = 0;
						for (int i = 0; i < 8; i++)
						{
							byte_recieved *= 2;
							byte_recieved += bits_recieved[i];
						}
						printf(">Recieved %d\n", byte_recieved);
					}
				}
				
			}
			else
			{
				if (sync_counted == 0)
				{
					sync_counted = 1;
					printf("Sync lost.\n");
				}
			}
			if (byte_recieved != 0 && byte_recieved != previous_byte_recieved)
			{
				printf("%d\n", byte_recieved);
				previous_byte_recieved = byte_recieved;
				/*cvPutText(frame, _itoa(byte_recieved, NULL, 10) , cvPoint(30, 120),
					&main_font, cvScalar(255, 255, 255, 0));*/
			}
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


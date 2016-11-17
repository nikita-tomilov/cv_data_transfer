#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define DOWNSCALE_W 800
#define DOWNSCALE_H 600

#define DOWNSCALE_SCALE 2

union CustomData {
	char bytes[12];
	float ints[3];
};

typedef struct
{
	float x;
	float y;
	float r;
} Circle_t;

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Pixel_t;

Circle_t* getAllCirlces(IplImage* img, int* n)
{
	int i;
	CvSeq* hc_out_seq;
	CvMemStorage* hc_out_mem = cvCreateMemStorage(0);
	*n = 0;

	hc_out_seq = cvHoughCircles(img, hc_out_mem, CV_HOUGH_GRADIENT, 1,
		1, // change this value to detect circles with different distances to each other
		100, 20, 1, 50 // change the last two parameters
		// (min_radius & max_radius) to detect larger circles
		);
	//printf("%f\n", *((float*)hc_out_seq->ptr));
	CvSeqReader reader;
	union CustomData val;
	cvStartReadSeq(hc_out_seq, &reader, 0);

	*n = hc_out_seq->total;

	Circle_t* tmp_array = (Circle_t*)malloc(*n * sizeof(Circle_t));
	Circle_t tmp_entry;

	//printf("%d\n", hc_out_seq->elem_size);
	for (i = 0; i < hc_out_seq->total; i++)
	{
		CV_READ_SEQ_ELEM(val, reader);
		tmp_entry.x = val.ints[0];
		tmp_entry.y = val.ints[1];
		tmp_entry.r = val.ints[2];
		tmp_array[i] = tmp_entry;
	}
	return tmp_array;
}

Pixel_t getPixel(IplImage* img, int x, int y)
{
	Pixel_t px;
	uchar* ptr = (uchar*)(img->imageData + y * img->widthStep);
	px.b = ptr[3 * x];
	px.g = ptr[3 * x + 1];
	px.r = ptr[3 * x + 2];
	return px;
}

void setPixel(IplImage* img, int x, int y, Pixel_t px)
{
	uchar* ptr = (uchar*)(img->imageData + y * img->widthStep);
	ptr[3 * x] = px.b;
	ptr[3 * x + 1] = px.g;
	ptr[3 * x + 2] = px.r;
}

/*int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return a < b ? a : b; }*/

Pixel_t calculateErodePixel(IplImage* img, int x, int y, int radius)
{
	int i, j;
	Pixel_t tmp, cur;
	tmp.r = 255;
	tmp.g = 255;
	tmp.b = 255;
	for (i = max(y - radius, 0); i <= min(y + radius, img->height - 1); i++)
		for (j = max(x - radius, 0); j <= min(x + radius, img->width - 1); j++)
		{
			cur = getPixel(img, j, i);
			tmp.r = min(tmp.r, cur.r);
			tmp.g = min(tmp.g, cur.g);
			tmp.b = min(tmp.b, cur.b);
		}
	return tmp;
}

Pixel_t calculateSaturationPixel(IplImage* img, int x, int y, int radius)
{
	Pixel_t cur = getPixel(img, x, y);
	cur.r = min((int)(cur.r * 1.0 * radius / 100), 255);
	return cur;
}

void applyFuncOnImage(IplImage* src, IplImage* dest, uint32_t radius, Pixel_t(pixelfunc)(IplImage*, int, int, int))
{
	int x, y;
	Pixel_t px;
	px.r = 255;
	px.g = 0;
	px.b = 0;
	for (y = 0; y < dest->height; y++)
	{
		uchar* ptr = (uchar*)(dest->imageData + y * dest->widthStep);
		for (x = 0; x < dest->width; x++)
		{
			px = pixelfunc(src, x, y, radius);
			ptr[3 * x] = px.b;
			ptr[3 * x + 1] = px.g;
			ptr[3 * x + 2] = px.r;
		}
	}
}

int main(int argc, char* argv[])
{
	CvCapture* capture = cvCreateCameraCapture(CV_CAP_ANY); //cvCaptureFromCAM( 0 );
	assert(capture);

	cvNamedWindow("capture", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("settings", CV_WINDOW_AUTOSIZE);

	IplImage* frame = cvQueryFrame(capture);

	IplImage* dst = 0; // cvCreateImage(cvSize(frame->width / DOWNSCALE_SCALE, frame->height / DOWNSCALE_SCALE), frame->depth, frame->nChannels);
	//cvResize(frame, dst, CV_INTER_LINEAR);
	IplImage* bw = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
	IplImage* hls = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels);
	//frame = cvClone(dst);

	int sat = 150;
	cvCreateTrackbar("Saturation", "settings", &sat, 1000, NULL);

	while (1)
	{
		// получаем кадр
		frame = cvQueryFrame(capture);
		dst = cvClone(frame);
		cvCvtColor(frame, bw, CV_BGR2GRAY);

		cvCvtColor(frame, hls, CV_BGR2HLS);

		applyFuncOnImage(hls, hls, sat, calculateSaturationPixel);

		cvCvtColor(hls, dst, CV_HLS2BGR);

		//cvEqualizeHist(frame, dst);
		/*
		int n = 0;
		Circle_t* cir, cur;
		cir = getAllCirlces(bw, &n);
		if (n > 10) n = 10;

		for (int i = 0; i < n; i++)
		{
		cur = cir[i];
		cvCircle(dst, cvPoint(cur.x, cur.y), cur.r, cvScalar(0, 0, 255,0), 1, 8, 0);
		}*/

		// показываем
		cvShowImage("capture", dst);

		char c = cvWaitKey(33);
		if (c == 27) { // нажата ESC
			break;
		}
	}
}


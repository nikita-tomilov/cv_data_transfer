#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_filters.h"

int abs(int x)
{
	return x < 0 ? -1 * x : x;
}

/* used to get pixel from image */
struct Pixel_t getPixel(IplImage* img, int x, int y)
{
	struct Pixel_t px;
	uchar* ptr = (uchar*)(img->imageData + y * img->widthStep);
	px.b = ptr[3 * x];
	px.g = ptr[3 * x + 1];
	px.r = ptr[3 * x + 2];
	return px;
}

/* used to set pixel in image */
void setPixel(IplImage* img, int x, int y, struct Pixel_t px)
{
	uchar* ptr = (uchar*)(img->imageData + y * img->widthStep);
	ptr[3 * x] = px.b;
	ptr[3 * x + 1] = px.g;
	ptr[3 * x + 2] = px.r;
}

static int mmax(int a, int b)
{
	return (a > b ? a : b);
}
static int mmin(int a, int b)
{
	return (a < b ? a : b);
}

/* used to calculate erode pixel in image */
/* uses no arguments */
struct Pixel_t calculateErodePixel(IplImage* img, int x, int y, int argc, int* argv)
{
	int i, j;
	int radius = argv[0];
	struct Pixel_t tmp, cur;
	tmp.r = 255;
	tmp.g = 255;
	tmp.b = 255;
	for (i = mmax(y - radius, 0); i <= mmin(y + radius, img->height - 1); i++)
		for (j = mmax(x - radius, 0); j <= mmin(x + radius, img->width - 1); j++)
		{
			cur = getPixel(img, j, i);
			tmp.r = mmin(tmp.r, cur.r);
			tmp.g = mmin(tmp.g, cur.g);
			tmp.b = mmin(tmp.b, cur.b);
		}
	return tmp;
}
/*
B = H
G = L
R = S
*/

/* used to make threshold on blue channel (blue or hue) */
/* uses two arguments - lower (0) and upper (1) bound */
struct Pixel_t calculateBthresholdPixel(IplImage* img, int x, int y, int argc, int* argv)
{
	struct Pixel_t cur = getPixel(img, x, y);
	if (cur.b >= argv[0] && cur.b <= argv[1]) return cur;
	cur.b = 0;
	cur.g = 0;
	cur.r = 0;
	return cur;
}

/* used to make threshold on RGB in required radius */
/* uses four arguments - r, g, b and radius in which point still counts */
struct Pixel_t calculatethresholdByRGBValue(IplImage* img, int x, int y, int argc, int* argv)
{
	struct Pixel_t cur = getPixel(img, x, y);
	struct Pixel_t ret;
	ret.b = 0;
	ret.g = 0;
	ret.r = 0;
	
	if (abs(cur.r - argv[0]) > argv[3]) return ret;
	if (abs(cur.g - argv[1]) > argv[4]) return ret;
	if (abs(cur.b - argv[2]) > argv[5]) return ret;

	return cur;
}

/* used to change saturation (or red channel) */
/* uses one argument - saturation percentage */
struct Pixel_t calculateSaturationPixel(IplImage* img, int x, int y, int argc, int* argv)
{
	struct Pixel_t cur = getPixel(img, x, y);
	cur.r = mmin((int)(cur.r * 1.0 * argv[0] / 100), 255);
	return cur;
}

/* applies function on whole image */
/* recieves pointer on pixel calculation and invokes it for each pixel */
void applyFuncOnImage(IplImage* src, IplImage* dest, int argc, int* argv, struct Pixel_t(pixelfunc)(IplImage*, int, int, int, int*))
{
	int x, y;
	struct Pixel_t px;
	px.r = 255;
	px.g = 0;
	px.b = 0;
	for (y = 0; y < dest->height; y++)
	{
		uchar* ptr = (uchar*)(dest->imageData + y * dest->widthStep);
		for (x = 0; x < dest->width; x++)
		{
			px = pixelfunc(src, x, y, argc, argv);
			ptr[3 * x] = px.b;
			ptr[3 * x + 1] = px.g;
			ptr[3 * x + 2] = px.r;
		}
	}
}

#ifndef _IMG_FILTERS_H
#define _IMG_FILTERS_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* used for image filters */
struct Pixel_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/* used to get pixel from image */
struct Pixel_t getPixel(IplImage* img, int x, int y);

/* used to set pixel in image */
void setPixel(IplImage* img, int x, int y, struct Pixel_t px);

/* applies function on whole image */
/* recieves pointer on pixel calculation and invokes it for each pixel */
void applyFuncOnImage(IplImage* src, IplImage* dest, int argc, int* argv, struct Pixel_t(pixelfunc)(IplImage*, int, int, int, int*));

/* used to make treshold on RGB in required radius */
/* uses four arguments - r, g, b and radius in which point still counts */
struct Pixel_t calculateTresholdByRGBValue(IplImage* img, int x, int y, int argc, int* argv);

#endif
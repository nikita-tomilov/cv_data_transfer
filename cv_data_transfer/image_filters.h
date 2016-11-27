#ifndef _IMG_FILTERS_H
#define _IMG_FILTERS_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* used for image filters */
typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Pixel_t;

/* used to get pixel from image */
Pixel_t getPixel(IplImage* img, int x, int y);

/* used to set pixel in image */
void setPixel(IplImage* img, int x, int y, Pixel_t px);

/* applies function on whole image */
/* recieves pointer on pixel calculation and invokes it for each pixel */
void applyFuncOnImage(IplImage* src, IplImage* dest, int argc, int* argv, Pixel_t(pixelfunc)(IplImage*, int, int, int*));

/* used to make treshold on RGB in required radius */
/* uses four arguments - r, g, b and radius in which point still counts */
Pixel_t calculateTresholdByRGBValue(IplImage* img, int x, int y, int argc, int* argv);

#endif
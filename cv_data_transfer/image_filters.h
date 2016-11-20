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

#endif
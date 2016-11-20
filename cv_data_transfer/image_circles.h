#ifndef _IMG_CIRCLES_H
#define _IMG_CIRCLES_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* used to retrieve founded circles data */
union CustomData {
	char bytes[12];
	float ints[3];
};

/* used for founded circle info */
typedef struct
{
	float x;
	float y;
	float r;
} Circle_t;

/* used to retrieve all circles from image */
/* returns array of circles and stores the amount of them in n */
Circle_t* getAllCirlces(IplImage* img, int* n);

/* used to retrieve uniq circles from image */
/* returns array of circles and stores the amount of them in outputAmount */
Circle_t* getUniqCircles(Circle_t* inputArray, int inputAmount, int minRadius, int* outputAmount);

#endif
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
Circle_t* getAllCirlces(IplImage* img, size_t* n);

/* used to retrieve uniq circles from image */
/* returns array of circles and stores the amount of them in outputAmount */
Circle_t* getUniqCircles(Circle_t* inputArray, size_t inputAmount, int minRadius, size_t* outputAmount);

/* used to define circles on image for their easier usage */
void calculateCircles(Circle_t* array, Circle_t* top_left, Circle_t* top_right, Circle_t* bottom_left, Circle_t* sync, Circle_t* data);

/* returns 1 if bit represented by circle is set to 1 and 0 otherwise */
int bitSet(IplImage* image, Circle_t* circle, float treshold);

#endif
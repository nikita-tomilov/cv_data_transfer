#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"
#include "image_filters.h"



static int square(int x)
{
	return x * x;
}

/* used to retrieve all circles from image */
/* returns array of circles and stores the amount of them in n */
void getAllCirlces(IplImage* img, struct Circle_t* buf, size_t* n)
{
	int i;
	CvSeq* hc_out_seq;
	CvMemStorage* hc_out_mem = cvCreateMemStorage(0);
	*n = 0;

	hc_out_seq = cvHoughCircles(img, hc_out_mem, CV_HOUGH_GRADIENT, 1,
		1, /*change this value to detect circles with different distances to each other */
		100, 20, 0, 20 /* change the last two parameters
						 (min_radius & max_radius) to detect larger circles */
		);
	
	CvSeqReader reader;
	union CustomData val;
	cvStartReadSeq(hc_out_seq, &reader, 0);

	*n = (size_t) hc_out_seq->total;

	/* struct Circle_t* tmp_array = (struct Circle_t*)malloc(*n * sizeof(struct Circle_t));  //TODO: NO MALLOC */
	struct Circle_t tmp_entry;

	//printf("%d\n", hc_out_seq->elem_size);
	for (i = 0; (i < hc_out_seq->total) && (i < MAX_CIRCLES); i++)
	{
		CV_READ_SEQ_ELEM(val, reader);
		tmp_entry.x = val.ints[0];
		tmp_entry.y = val.ints[1];
		tmp_entry.r = val.ints[2];
		buf[i] = tmp_entry;
	}
	return;
}

/* used to retrieve uniq circles from image */
/* returns array of circles and stores the amount of them in outputAmount */

void getUniqCircles(struct Circle_t* inputArray, size_t inputAmount, int minRadius, struct Circle_t* outputArray, size_t* outputAmount)
{
	size_t n = 0;
	size_t i = 0;
	size_t j = 0;
	int f = 1;

	/* struct Circle_t* tmpArray = (struct Circle_t*)malloc(inputAmount * sizeof(struct Circle_t)); */

	for (i = 0; i < inputAmount; i++)
	{
		f = 1;
		for (j = 0; (j < n) && (j < MAX_UNIQ_CIRCLES); j++)
		{
			if ((square((int) (inputArray[i].x - outputArray[j].x))) + square((int) (inputArray[i].y - outputArray[j].y)) < minRadius * minRadius) f = 0;

		}
		if (f)
		{
			n++;
			outputArray[j] = inputArray[i];
		}
	}
	*outputAmount = n;
	return;
}

/* used to define circles on image for their easier usage */

void calculateCircles(struct Circle_t* array, struct Circle_t* top_left, struct Circle_t* top_right, struct Circle_t* bottom_left, struct Circle_t* sync, struct Circle_t* data)
{
	struct Circle_t middle;
	size_t i;
	middle.x = ((array[0].x) + (array[1].x) + (array[2].x)) / 3.0f;
	middle.y = ((array[0].y) + (array[1].y) + (array[2].y)) / 3.0f;
	middle.r = ((array[0].r) + (array[1].r) + (array[2].r)) / 3.0f;
	*top_left = middle;
	*top_right = middle;
	*bottom_left = middle;
	*sync = middle;
	*data = middle;

	for (i = 0; i < 3; i++)
	{
		if (array[i].x <= top_left->x && array[i].y <= top_left->y) *top_left = array[i];
		if (array[i].x >= top_right->x && array[i].y <= top_right->y) *top_right = array[i];
		if (array[i].x <= bottom_left->x && array[i].y >= bottom_left->y) *bottom_left = array[i];
	}


	data->x = (top_right->x + bottom_left->x) / 2;
	data->y = (top_right->y + bottom_left->y) / 2;

	sync->x = (2 * data->x - top_left->x);
	sync->y = (2 * data->y - top_left->y);
	
}

/* returns 1 if bit represented by circle is set to 1 and 0 otherwise */

int bitSet(IplImage* image, struct Circle_t* circle, float treshold)
{
	struct Pixel_t px;
	float curval;
	px = getPixel(image, (int) circle->x, (int) circle->y);
	curval = (px.r + px.g + px.b) / 3.0f;
	if (curval >= treshold) return (int)curval;
	return 0;
}

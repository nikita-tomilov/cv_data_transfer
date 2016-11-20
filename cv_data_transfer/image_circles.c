#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"


static int square(int x)
{
	return x * x;
}

/* used to retrieve all circles from image */
/* returns array of circles and stores the amount of them in n */
Circle_t* getAllCirlces(IplImage* img, int* n)
{
	int i;
	CvSeq* hc_out_seq;
	CvMemStorage* hc_out_mem = cvCreateMemStorage(0);
	*n = 0;

	hc_out_seq = cvHoughCircles(img, hc_out_mem, CV_HOUGH_GRADIENT, 1,
		1, // change this value to detect circles with different distances to each other
		100, 20, 0, 20 // change the last two parameters
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

/* used to retrieve uniq circles from image */
/* returns array of circles and stores the amount of them in outputAmount */

Circle_t* getUniqCircles(Circle_t* inputArray, int inputAmount, int minRadius, int* outputAmount)
{
	int n = -1;
	int i = 0;
	int j = 0;
	int f = 1;

	Circle_t* tmpArray = (Circle_t*)malloc(inputAmount * sizeof(Circle_t));

	for (i = 0; i < inputAmount; i++)
	{
		f = 1;
		for (j = 0; j < n; j++)
		{
			if (square(inputArray[i].x - tmpArray[j].x) + square(inputArray[i].y - tmpArray[j].y) < minRadius * minRadius) f = 0;

		}
		if (f)
		{
			n++;
			tmpArray[j] = inputArray[i];
		}
	}
	*outputAmount = n;
	return tmpArray;
}
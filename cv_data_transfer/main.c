#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"
#include "image_filters.h"

#define MIN_CIRCLE_RADIUS 15

/* used to treshold on blue tracking points */
/* b, g, r, radius */
int g_tracking_values[] = { 0, 0, 0, 5, 5, 5 };

/* main frame for captured image */
IplImage* frame;

/* flag that transfer aborted */
int is_abort_pressed = 0;


/* used to get bit state in a graph */
int getBitState(int* array, size_t delta)
{
	size_t i;
	for (i = 0; i < delta; i++)
	{
		if (array[255 - i] <= 10) return 0; //hole => bit not set now
	}
	for (i = 0; i < delta; i++)
	{
		array[255 - i] = 0;
	}
	return 1;
}

/* used to track mouse clicks and adjust color sliders */
void mouseCallback(int mevent, int x, int y, int flags, void* userdata)
{
	if (mevent == CV_EVENT_LBUTTONUP)
	{
		struct Pixel_t px;
		px = getPixel(frame, x, y);
		g_tracking_values[0] = px.r;
		g_tracking_values[1] = px.g;
		g_tracking_values[2] = px.b;
		/* puts("lmouse %d %d", x, y); */
	}
	if (mevent == CV_EVENT_RBUTTONUP)
	{
		is_abort_pressed = 1;
	}
}


/* launch params structure and special function to parse that stuff */
struct launch_params_t
{

	/* program settings */
	int show_all_debug;
	int write_to_file;

	/* file IO */
	FILE* output_file;

};
struct launch_params_t getLaunchParams(int argc, char* argv[])
{
	size_t i;
	struct launch_params_t current;

	current.output_file = NULL;
	current.show_all_debug = 0;
	current.write_to_file = 0;

	/* WAIT! Let's check program launch params */
	for (i = 0; i < argc; i++)
	{
		/* puts(">> %s", argv[i]); */
		if (strcmp(argv[i], "-h") == 0)
		{
			puts("cv_data_transfer, PC side\
				 https://github.com/Programmer74/cv_data_transfer \
				Additional parameters: \
				-h Show this help \
				-o <filename> - Write recieved bytes to file \
				-v Show all logs");
			exit(0);
		}
		if (strcmp(argv[i], "-v") == 0) current.show_all_debug = 1;
		if (strcmp(argv[i], "-o") == 0)
		{
			if (i + 1 == argc)
			{
				puts("No filename specified.");
				exit(0);
			}
			current.output_file = fopen(argv[i + 1], "wb");
			if (current.output_file == NULL)
			{
				printf("Cannot open file %s.\n", argv[i + 1]);
				exit(0);
			}
			current.write_to_file = 1;
			printf("Enabled data output to file %s.\n", argv[i + 1]);
		}
	}
	return current;
}

/* opencv stuff structure and opencv gui initialising*/
struct opencv_stuff_t
{
	/* font for drawing */
	CvFont main_font;

	/* time to track circles in "ticks"
	one tick = one frame being captured */
	int tracking_delay;

	/* images */
	IplImage* dst;
	IplImage* bw;
	/* capture device */
	CvCapture* capture;
};
struct opencv_stuff_t initOpenCVGui()
{
	struct opencv_stuff_t current;
	/* UI AND OPENCV */
	/* initialising camera */
	current.capture = cvCreateCameraCapture(CV_CAP_ANY);
	assert(current.capture);

	current.tracking_delay = 3; /* 3 frames */

	/* initialising window */
	cvNamedWindow("settings", CV_WINDOW_NORMAL);
	cvResizeWindow("settings", 400, 300);
	cvNamedWindow("filtered", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("capture", CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback("capture", mouseCallback, NULL);

	/* initialising images */
	frame = cvQueryFrame(current.capture); /* webcam image */
	current.dst = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels);  /*destination for all magic */
	current.bw = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1); /* bw image used for finding circles */

	/* initialising font for printing text */
	cvInitFont(&(current.main_font), CV_FONT_HERSHEY_COMPLEX_SMALL, 1.0f, 1.0f, 0, 1, CV_AA);

	/* adjusting trackbars */
	cvCreateTrackbar("R", "settings", g_tracking_values, 255, NULL);
	cvCreateTrackbar("G", "settings", g_tracking_values + 1, 255, NULL);
	cvCreateTrackbar("B", "settings", g_tracking_values + 2, 255, NULL);
	cvCreateTrackbar("Radius R", "settings", g_tracking_values + 3, 255, NULL);
	cvCreateTrackbar("Radius G", "settings", g_tracking_values + 4, 255, NULL);
	cvCreateTrackbar("Radius B", "settings", g_tracking_values + 5, 255, NULL);
	cvCreateTrackbar("Tracking delay", "settings", &(current.tracking_delay), 10, NULL);

	return current;
}

/* updates trackbars for current colors */
void updateTrackbars()
{
	cvSetTrackbarPos("R", "settings", g_tracking_values[0]);
	cvSetTrackbarPos("G", "settings", g_tracking_values[1]);
	cvSetTrackbarPos("B", "settings", g_tracking_values[2]);
}

/* */
int main(int argc, char* argv[])
{
	/* VARIABLES */

	/* iterators */
	size_t i;


	/* initialising markers, sync and data circles */
	struct Circle_t old_circles[3];
	int found_data_frame = 0;
	struct Circle_t top_left;
	struct Circle_t top_right;
	struct Circle_t bottom_left;
	struct Circle_t sync, data;

	/* all and uniq circles in locating data frame */
	size_t all_circles_count, uniq_circles_count;
	struct Circle_t all_circles[MAX_CIRCLES];
	struct Circle_t uniq_circles[MAX_UNIQ_CIRCLES];
	struct Circle_t current_circle;
	CvPoint centers[3];

	/* buffers for tracked values */
	int* sync_buf = (int*)calloc(256, sizeof(int));
	int* data_buf = (int*)calloc(256, sizeof(int));
	/* and backups to draw graph */
	int* sync_buf_bckp = (int*)calloc(256, sizeof(int));
	int* data_buf_bckp = (int*)calloc(256, sizeof(int));
	/* states */
	int sync_state = 0;
	int data_state = 0;
	/* timeouts */
	int sync_timeout = 0;
	int data_timeout = 0;

	/* recieved data parsing */
	int* recieved_bits = (int*)calloc(9, sizeof(int)); /* 8 + parity */
	int recieved_bits_count = 0;
	int is_data_transferring = 0;
	uint8_t incoming_value = 0;
	int current_parity = 0;
	
	/* parsing launch params */
	struct launch_params_t current_params = getLaunchParams(argc, argv);
	
	puts("cv_data_transfer successfully launched.");

	/* initialising GUI & opencv stuff */
	struct opencv_stuff_t opencv_vars = initOpenCVGui();

	/* MAIN LOOP GOES HERE */
	while (1)
	{
		updateTrackbars();

		/* retrieving image */
		frame = cvQueryFrame(opencv_vars.capture);
		opencv_vars.dst = cvClone(frame);

		/* filtering by required params*/
		applyFuncOnImage(frame, opencv_vars.dst, 4, g_tracking_values, calculateTresholdByRGBValue);
		/* creating and adjusting mask */
		cvCvtColor(opencv_vars.dst, opencv_vars.bw, CV_BGR2GRAY);
		cvSmooth(opencv_vars.bw, opencv_vars.bw, CV_GAUSSIAN, 15, 0, 3, 3);
		cvDilate(opencv_vars.bw, opencv_vars.bw, cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE, NULL), 2);

		/* drawing all uniq circles */
		all_circles_count = 0;
		uniq_circles_count = 0;

		getAllCirlces(opencv_vars.bw, all_circles, &all_circles_count);
		getUniqCircles(all_circles, all_circles_count, MIN_CIRCLE_RADIUS, uniq_circles, &uniq_circles_count);

		if (uniq_circles_count > 10) uniq_circles_count = 10;

		if (uniq_circles_count == 3)
		{
			found_data_frame = 1;


			cvPutText(frame, "Data Frame recognized.", cvPoint(30, 30),
				&(opencv_vars.main_font), cvScalar(0, 255, 0, 0));

			calculateCircles(uniq_circles, &top_left, &top_right, &bottom_left, &sync, &data);

			for (i = 0; i < 3; i++)
			{
				current_circle = uniq_circles[i];
				centers[i].x = current_circle.x;
				centers[i].y = current_circle.y;
				old_circles[i] = uniq_circles[i];
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
				&(opencv_vars.main_font), cvScalar(0, 0, 255, 0));
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

		/* getting ready to draw fancy graph */
		for (i = 0; i < 255; i++)
		{
			sync_buf[i] = sync_buf[i + 1];
			data_buf[i] = data_buf[i + 1];
			sync_buf_bckp[i] = sync_buf_bckp[i + 1];
			data_buf_bckp[i] = data_buf_bckp[i + 1];
		}

		if (found_data_frame)
		{
			sync_buf[255] = bitSet(frame, &sync, 230);
			data_buf[255] = bitSet(frame, &data, 230);
			sync_buf_bckp[255] = sync_buf[255];
			data_buf_bckp[255] = data_buf[255];
			if (sync_buf[255])
			{
				cvPutText(frame, "Sync bit is enabled.", cvPoint(30, 60),
					&(opencv_vars.main_font), cvScalar(255, 0, 0, 0));

				if (data_buf[255])
				{
					cvPutText(frame, "Data bit is enabled.", cvPoint(30, 90),
						&(opencv_vars.main_font), cvScalar(255, 255, 255, 0));
				}
			}

		}

		/* drawing fancy graph */
		for (i = 1; i < 256; i++)
		{
			cvDrawLine(frame, cvPoint(i * 2, 350 - sync_buf_bckp[i - 1] / 4), cvPoint(i * 2 + 1, 350 - sync_buf_bckp[i] / 4), cvScalar(0, 0, 255, 0), 1, 1, 0);
			cvDrawLine(frame, cvPoint(i * 2, 450 - data_buf_bckp[i - 1] / 4), cvPoint(i * 2 + 1, 450 - data_buf_bckp[i] / 4), cvScalar(255, 0, 0, 0), 1, 1, 0);
			if (i % 2 == 0) cvDrawLine(frame, cvPoint(i * 2, 400), cvPoint(i * 2 + 1, 400), cvScalar(255, 255, 255, 0), 1, 1, 0);
		}

		/* now that we have drawn graph we are free to analyze its results */
		sync_state = getBitState(sync_buf, opencv_vars.tracking_delay);
		data_state = getBitState(data_buf, opencv_vars.tracking_delay);

		/* analysing image begins */
		if (sync_timeout > 0) sync_timeout--;
		if (data_timeout > 0) data_timeout--;

		if (sync_state && (data_state == 0))
		{
			if (current_params.show_all_debug) puts("Only sync enabled.");
			sync_timeout = 10;
			if (is_data_transferring)
			{
				recieved_bits[recieved_bits_count] = 0;
				recieved_bits_count++;
			}
		}
		if (!sync_state && data_state)
		{
			if (current_params.show_all_debug) puts("Only data enabled.");
			data_timeout = 10;
			if (sync_timeout > 0 && !is_data_transferring)
			{
				puts("Got starting marker; transferring began");
				is_data_transferring = 1;
				recieved_bits_count = 0;
			}
		}
		if (sync_state && data_state)
		{
			if (current_params.show_all_debug) puts("Both are enabled.");
			if (is_data_transferring)
			{
				recieved_bits[recieved_bits_count] = 1;
				recieved_bits_count++;
			}
		}

		/* byte recieved */
		if (recieved_bits_count == 9)
		{
			printf("Got byte 0b"); /* has to be printf for no \n on the end of string */
			current_parity = 0;
			incoming_value = 0;
			is_data_transferring = 0;
			for (i = 0; i < 8; i++)
			{
				printf("%d", recieved_bits[i]);
				incoming_value = incoming_value * 2 + recieved_bits[i];
				current_parity += recieved_bits[i];
			}
			printf("\n> Parity recieved is %d.\n", recieved_bits[8]);
			current_parity = (current_parity % 2 == 0 ? 0 : 1);
			if (current_parity == recieved_bits[8])
			{
				if (current_params.show_all_debug) puts("> Logged to file.");
				printf("> Recieved byte %d == '%c'\n", incoming_value, (char)incoming_value);
				if (current_params.write_to_file)
				{
					puts("> Parity OK.");
					fwrite(&incoming_value, sizeof(uint8_t), 1, current_params.output_file);
					//fseek(output_file, sizeof(uint8_t), );
				}
			}
			else
			{
				puts("> Parity FAILED");
			}
			recieved_bits_count = 0;
			puts("");
		}

		/* if abort pressed */
		if (is_abort_pressed)
		{
			puts("DATA TRANSFER ABORTED.");
			recieved_bits_count = 0;
			is_abort_pressed = 0;
		}

		/* showing image */
		cvShowImage("capture", frame);
		cvShowImage("filtered", opencv_vars.bw);

		char c = cvWaitKey(33);
		if (c == 27) { /* esc key pressed */
			break;

		}

		/* freeing up */
		cvReleaseImage(&(opencv_vars.dst));
		/* should NOT release bw and frame */

	}
	puts("Closed.");

	cvReleaseCapture(&(opencv_vars.capture));
	if (current_params.write_to_file)
	{
		puts("File closing.");
		fclose(current_params.output_file);
	}
	puts("Bye.");
	return 0;
}

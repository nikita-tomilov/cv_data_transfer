#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image_circles.h"
#include "image_filters.h"
#include "getopt.h"

#define MIN_CIRCLE_RADIUS 15

/* used to threshold on blue tracking points */
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
	struct launch_params_t current;
	int optres;

	current.output_file = NULL;
	current.show_all_debug = 0;
	current.write_to_file = 0;

	opterr = 0;
	while ((optres = getopt(argc, argv, "hvo:")) != -1)
	{
		switch (optres)
		{
			case 'h':
				puts("\n\
					 cv_data_transfer, PC side\n\
					 https://github.com/Programmer74/cv_data_transfer \n\
					 Additional parameters: \n\
					 -h Show this help \n\
					 -o <filename> - Write received bytes to file \n\
					 -v Show all logs\n");
				exit(0);
			case 'v': 
				current.show_all_debug = 1; 
				break;
			case 'o': 
				current.output_file = fopen(optarg, "wb");
				if (current.output_file == NULL)
				{
					printf("Cannot open file %s.\n", optarg);
					exit(0);
				}
				current.write_to_file = 1;
				printf("Enabled data output to file %s.\n", optarg);
				break;

			case '?': printf("Wrong parameter specified!\n"); exit(0);
		};
	};
	return current;

}

/* opencv stuff structure and opencv gui initialising */
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
	cvInitFont(&(current.main_font), CV_FONT_HERSHEY_COMPLEX_SMALL, 1.0, 1.0, 0, 1, CV_AA);

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

/* structure to hold circles and void to draw them */
struct opencv_circles_t
{
	struct Circle_t old_circles[3];
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
};
void parseAndDrawCircles(struct opencv_stuff_t opencv_vars, struct opencv_circles_t* circles, int found_data_frame)
{
	size_t i;
	if (circles->uniq_circles_count == 3)
	{
		cvPutText(frame, "Data Frame recognized.", cvPoint(30, 30),
			&(opencv_vars.main_font), cvScalar(0, 255, 0, 0));

		calculateCircles(circles->uniq_circles, &(circles->top_left), &(circles->top_right), &(circles->bottom_left), &(circles->sync), &(circles->data));

		for (i = 0; i < 3; i++)
		{
			circles->current_circle = circles->uniq_circles[i];
			circles->centers[i].x = (int) circles->current_circle.x;
			circles->centers[i].y = (int) circles->current_circle.y;
			circles->old_circles[i] = circles->uniq_circles[i];
		}

		/* drawing markers */
		cvCircle(frame, cvPoint((int)circles->top_left.x, (int)circles->top_left.y), (int)circles->top_left.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
		cvCircle(frame, cvPoint((int)circles->top_right.x, (int)circles->top_right.y), (int)circles->top_right.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
		cvCircle(frame, cvPoint((int)circles->bottom_left.x, (int)circles->bottom_left.y), (int)circles->bottom_left.r, cvScalar(0, 0, 255, 0), 1, 8, 0);
		/* drawing sync and data */
		cvCircle(frame, cvPoint((int)circles->data.x, (int)circles->data.y), (int)circles->data.r, cvScalar(255, 255, 255, 0), 1, 8, 0);
		cvCircle(frame, cvPoint((int)circles->sync.x, (int)circles->sync.y), (int)circles->sync.r, cvScalar(255, 0, 0, 0), 1, 8, 0);
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
			cvCircle(frame, cvPoint((int)circles->top_left.x, (int)circles->top_left.y), (int)circles->top_left.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
			cvCircle(frame, cvPoint((int)circles->top_right.x, (int)circles->top_right.y), (int)circles->top_right.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
			cvCircle(frame, cvPoint((int)circles->bottom_left.x, (int)circles->bottom_left.y), (int)circles->bottom_left.r, cvScalar(0, 0, 128, 0), 1, 8, 0);
			/* drawing sync and data */
			cvCircle(frame, cvPoint((int)circles->data.x, (int)circles->data.y), (int)circles->data.r, cvScalar(128, 128, 128, 0), 1, 8, 0);
			cvCircle(frame, cvPoint((int)circles->sync.x, (int)circles->sync.y), (int)circles->sync.r, cvScalar(128, 128, 128, 0), 1, 8, 0);
		}
	}

}

/* structure to hold data parsing buffers, void to init them */
struct opencv_dataparsebuffs_t
{
	/* buffers for tracked values */
	int* sync_buf;
	int* data_buf;
	/* and backups to draw graph */
	int* sync_buf_bckp;
	int* data_buf_bckp;
	/* states */
	int sync_state;
	int data_state;
	/* timeouts */
	int sync_timeout;
	int data_timeout;

	/* received data parsing */
	int* recieved_bits;
	int recieved_bits_count;
	int is_data_transferring;
	uint8_t incoming_value;
	int current_parity;
};
struct opencv_dataparsebuffs_t initDataParsingBuffers()
{
	struct opencv_dataparsebuffs_t current;
	/* buffers for tracked values */
	current.sync_buf = (int*)calloc(256, sizeof(int));
	current.data_buf = (int*)calloc(256, sizeof(int));
	/* and backups to draw graph */
	current.sync_buf_bckp = (int*)calloc(256, sizeof(int));
	current.data_buf_bckp = (int*)calloc(256, sizeof(int));
	/* states */
	current.sync_state = 0;
	current.data_state = 0;
	/* timeouts */
	current.sync_timeout = 0;
	current.data_timeout = 0;

	/* received data parsing */
	current.recieved_bits = (int*)calloc(9, sizeof(int)); /* 8 + parity */
	current.recieved_bits_count = 0;
	current.is_data_transferring = 0;
	current.incoming_value = 0;
	current.current_parity = 0;

	return current;
}

/* void to capture actual circles data and draw fancy captured brightness graph */
void captureAndDrawFancyGraph(struct opencv_stuff_t opencv_vars, struct opencv_circles_t circles, struct opencv_dataparsebuffs_t* databuffs, int found_data_frame)
{
	size_t i;

	/* getting ready to draw fancy graph */
	for (i = 0; i < 255; i++)
	{
		databuffs->sync_buf[i] = databuffs->sync_buf[i + 1];
		databuffs->data_buf[i] = databuffs->data_buf[i + 1];
		databuffs->sync_buf_bckp[i] = databuffs->sync_buf_bckp[i + 1];
		databuffs->data_buf_bckp[i] = databuffs->data_buf_bckp[i + 1];
	}

	if (found_data_frame)
	{
		databuffs->sync_buf[255] = bitSet(frame, &(circles.sync), 230);
		databuffs->data_buf[255] = bitSet(frame, &(circles.data), 230);
		databuffs->sync_buf_bckp[255] = databuffs->sync_buf[255];
		databuffs->data_buf_bckp[255] = databuffs->data_buf[255];
		if (databuffs->sync_buf[255])
		{
			cvPutText(frame, "Sync bit is enabled.", cvPoint(30, 60),
				&(opencv_vars.main_font), cvScalar(255, 0, 0, 0));

			if (databuffs->data_buf[255])
			{
				cvPutText(frame, "Data bit is enabled.", cvPoint(30, 90),
					&(opencv_vars.main_font), cvScalar(255, 255, 255, 0));
			}
		}

	}

	/* drawing fancy graph */
	for (i = 1; i < 256; i++)
	{
        /* WARNING: implicit conversion loses integer precision: 'unsigned long' to 'int' */
		cvDrawLine(frame, cvPoint(i * 2, 350 - databuffs->sync_buf_bckp[i - 1] / 4), cvPoint(i * 2 + 1, 350 - databuffs->sync_buf_bckp[i] / 4), cvScalar(0, 0, 255, 0), 1, 1, 0);
		cvDrawLine(frame, cvPoint(i * 2, 450 - databuffs->data_buf_bckp[i - 1] / 4), cvPoint(i * 2 + 1, 450 - databuffs->data_buf_bckp[i] / 4), cvScalar(255, 0, 0, 0), 1, 1, 0);
		if (i % 2 == 0) cvDrawLine(frame, cvPoint(i * 2, 400), cvPoint(i * 2 + 1, 400), cvScalar(255, 255, 255, 0), 1, 1, 0);
	}
}

/* void to analyze captured info */
void analyzeCapturedInfo(struct launch_params_t current_params, struct opencv_dataparsebuffs_t* databuffs)
{
	if (databuffs->sync_state && (databuffs->data_state == 0))
	{
		if (current_params.show_all_debug) puts("Only sync enabled.");
		databuffs->sync_timeout = 10;
		if (databuffs->is_data_transferring)
		{
			databuffs->recieved_bits[databuffs->recieved_bits_count] = 0;
			databuffs->recieved_bits_count++;
		}
	}
	if (!databuffs->sync_state && databuffs->data_state)
	{
		if (current_params.show_all_debug) puts("Only data enabled.");
		databuffs->data_timeout = 10;
		if (databuffs->sync_timeout > 0 && !databuffs->is_data_transferring)
		{
			puts("Got starting marker; transferring began");
			databuffs->is_data_transferring = 1;
			databuffs->recieved_bits_count = 0;
		}
	}
	if (databuffs->sync_state && databuffs->data_state)
	{
		if (current_params.show_all_debug) puts("Both are enabled.");
		if (databuffs->is_data_transferring)
		{
			databuffs->recieved_bits[databuffs->recieved_bits_count] = 1;
			databuffs->recieved_bits_count++;
		}
	}
}

/* void to parse and get byte from that captured info */
int getCapturedByte(struct launch_params_t current_params, struct opencv_dataparsebuffs_t* databuffs, uint8_t* recieved_byte)
{
	int found = 0;
	size_t i;
	printf("Got byte 0b"); /* has to be printf for no \n on the end of string */
	databuffs->current_parity = 0;
	databuffs->incoming_value = 0;
	databuffs->is_data_transferring = 0;
	for (i = 0; i < 8; i++)
	{
		printf("%d", databuffs->recieved_bits[i]);
        /* WARNING: implicit conversion loses integer precision: 'int' to 'uint8_t' (aka 'unsigned char') */
		databuffs->incoming_value = databuffs->incoming_value * 2 + databuffs->recieved_bits[i];
		databuffs->current_parity += databuffs->recieved_bits[i];
	}
	printf("\n> Parity received is %d.\n", databuffs->recieved_bits[8]);
	databuffs->current_parity = (databuffs->current_parity % 2 == 0 ? 0 : 1);
	if (databuffs->current_parity == databuffs->recieved_bits[8])
	{

		printf("> Received byte %d == '%c'\n", databuffs->incoming_value, (char)databuffs->incoming_value);
		puts("> Parity OK.");
		*recieved_byte = databuffs->incoming_value;
		found = 1;
	}
	else
	{
		puts("> Parity FAILED");
		found = 0;
	}
	databuffs->recieved_bits_count = 0;

	return found;
}

/* updates trackbars for current colors */
void updateTrackbars()
{
	cvSetTrackbarPos("R", "settings", g_tracking_values[0]);
	cvSetTrackbarPos("G", "settings", g_tracking_values[1]);
	cvSetTrackbarPos("B", "settings", g_tracking_values[2]);
}

/* MAIN STUFF */
int main(int argc, char* argv[])
{
	/* parsing launch params */
	struct launch_params_t current_params = getLaunchParams(argc, argv);
	
	puts("cv_data_transfer successfully launched.");

	/* initialising GUI & opencv stuff */
	struct opencv_stuff_t opencv_vars = initOpenCVGui();

	/* markers, sync and data circles are here in this struct, dont panic */
	struct opencv_circles_t circles;
	int found_data_frame = 0;

	/* while all that jazz for parsing and drawing data is in this struct */
	struct opencv_dataparsebuffs_t databuffs = initDataParsingBuffers();

	/* MAIN LOOP GOES HERE */
	while (1)
	{
		updateTrackbars();

		/* retrieving image */
		frame = cvQueryFrame(opencv_vars.capture);
		opencv_vars.dst = cvClone(frame);

		/* filtering by required params */
		applyFuncOnImage(frame, opencv_vars.dst, 4, g_tracking_values, calculateTresholdByRGBValue);
		/* creating and adjusting mask */
		cvCvtColor(opencv_vars.dst, opencv_vars.bw, CV_BGR2GRAY);
		cvSmooth(opencv_vars.bw, opencv_vars.bw, CV_GAUSSIAN, 15, 0, 3, 3);
		cvDilate(opencv_vars.bw, opencv_vars.bw, cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE, NULL), 2);

		/* capturing all uniq circles */
		circles.all_circles_count = 0;
		circles.uniq_circles_count = 0;

		getAllCirlces(opencv_vars.bw, circles.all_circles, &(circles.all_circles_count));
		getUniqCircles(circles.all_circles, circles.all_circles_count, MIN_CIRCLE_RADIUS, circles.uniq_circles, &(circles.uniq_circles_count));

		if (circles.uniq_circles_count > MAX_UNIQ_CIRCLES) circles.uniq_circles_count = MAX_UNIQ_CIRCLES;
		if (circles.uniq_circles_count == 3) found_data_frame = 1;

		/* drawing circles, parsing top/left/bottom/right circles */
		parseAndDrawCircles(opencv_vars, &circles, found_data_frame);

		/* capture actual data and draw it as a graph */
		captureAndDrawFancyGraph(opencv_vars, circles, &databuffs, found_data_frame);

		/* now that we have drawn graph we are free to analyze its results */
        /* WARNING: implicit conversion changes signedness: 'int' to 'size_t' (aka 'unsigned long') */
		databuffs.sync_state = getBitState(databuffs.sync_buf, opencv_vars.tracking_delay);
		databuffs.data_state = getBitState(databuffs.data_buf, opencv_vars.tracking_delay);

		/* analyzing that graph */
		if (databuffs.sync_timeout > 0) databuffs.sync_timeout--;
		if (databuffs.data_timeout > 0) databuffs.data_timeout--;
		analyzeCapturedInfo(current_params, &databuffs);
		
		/* byte received */
		if (databuffs.recieved_bits_count == 9)
		{
			uint8_t recieved;
			int result;
			
			result = getCapturedByte(current_params, &databuffs, &recieved);
			/* result already printed over there, but we need to save to file if needed */
			if (result != 0 && current_params.write_to_file)
			{
				fwrite(&recieved, sizeof(uint8_t), 1, current_params.output_file);
				if (current_params.show_all_debug) puts("> Logged to file.");
			}

		}

		/* if abort pressed */
		if (is_abort_pressed)
		{
			puts("DATA TRANSFER ABORTED.");
			databuffs.recieved_bits_count = 0;
			is_abort_pressed = 0;
		}

		/* showing image */
		cvShowImage("capture", frame);
		cvShowImage("filtered", opencv_vars.bw);

		int c = cvWaitKey(33);
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

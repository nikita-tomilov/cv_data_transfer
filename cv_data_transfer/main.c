/*Just a sample file to make sure everything goes OK*/

#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
        
        CvCapture* capture = cvCreateCameraCapture(CV_CAP_ANY); //cvCaptureFromCAM( 0 );
        assert( capture );

        double width = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
        double height = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
        printf("[i] %.0f x %.0f\n", width, height );

        IplImage* frame=0;
        IplImage* dst = 0;
        IplImage* dst2 = 0;

        cvNamedWindow("capture", CV_WINDOW_AUTOSIZE);

        printf("[i] press Enter for capture image and Esc for quit!\n\n");

        int counter=0;
        char filename[512];

        while(1){

                frame = cvQueryFrame( capture );

                dst2 = cvCreateImage( cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
                dst = cvCreateImage( cvSize(frame->width, frame->height), IPL_DEPTH_8U, 3);
                cvCanny(frame, dst2, 50, 200, 3);

                cvCvtColor(dst2, dst, CV_GRAY2RGB);
                cvSub(frame, dst, dst, NULL);
                cvShowImage("capture", dst);
                

                char c = cvWaitKey(33);
                if (c == 27) { 
                        break;
                }
                else if(c == 13) { 
                        
                        sprintf(filename, "Image%d.jpg", counter);
                        printf("[i] capture... %s\n", filename);
                        cvSaveImage(filename, frame, 0);
                        counter++;
                }
        }
        
        cvReleaseCapture( &capture );
        cvDestroyWindow("capture");
        return 0;
}


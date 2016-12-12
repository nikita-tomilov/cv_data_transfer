CC              := gcc
CFLAGS          := -IC:\opencv\build\include 
LIBPATH			:= C:\opencv\build\x86\vc11\lib
LIBRARIES       :=  opencv_core2413  opencv_calib3d2413  opencv_contrib2413  opencv_features2d2413  opencv_flann2413  opencv_highgui2413  opencv_imgproc2413  opencv_legacy2413  opencv_ml2413  opencv_nonfree2413  opencv_objdetect2413  opencv_photo2413  opencv_stitching2413  opencv_superres2413  opencv_ts2413  opencv_video2413  opencv_videostab2413
INCLUDE_LIBRARIES := $(foreach lib, $(LIBRARIES), $(LIBPATH)\$(lib).lib)

all: main

main: 
	$(CC) $(CFLAGS) -c main.c -o main.o
	$(CC) -L$(LIBPATH) -o main.exe main.o $(INCLUDE_LIBRARIES)
	
	   
clean:
	rm -f *.o

# cv_data_transfer

## Data transfer using Arduino and Webcam.
## Requirements:
- Arduino Uno/Leonardo/whatsoever with coloured TFT screen
- Web camera
- 2 PCs/Laptops/whatsoever
- OpenCV 2.4.13 on “reviever” PC

Other versions MAY WORK OR MAY NOT.

## How-to:
1. Program your Arduino with sketch in “arduino” folder
2. Connect it to “sender” PC
3. Open up whatever serial communication software you like (PuTTY etc.)
4. Make sure “sender” device actually sends something - the pattern on the screen changes
5. Compile and run “reciever” code on “reciever” PC
6. Point your “sender” device onto “reciever”’s webcam
7. Adjust sliders so PC can recognise markers on the device
8. Start sending bytes from “sender” and watch them appear on “reciever”

“Sender” device should look something like this:

![arduino_device](https://raw.githubusercontent.com/Programmer74/cv_data_transfer/master/device.jpg)

## Compilation

Windows with Visual Studio 2013 or later

- Download
- Make sure your OpenCV is in C:\opencv OR manually specify its destination in Project Preferences
- Make sure environment variable PATH points to OpenCV’s bin directory
- Compile, run, enjoy

Windows without VS2013, Linux, MacOS, other Unix-based OS

- Makefile coming soon
- Or copy-paste sources and run under your IDE, compilers, etc.
## Coming soon

No idea what to add =(



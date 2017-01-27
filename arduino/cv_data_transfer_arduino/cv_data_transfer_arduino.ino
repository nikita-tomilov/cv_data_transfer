/* The following code was taken "as-is" from the working demo. You'd better
 *  not to change unless it is not working.
 */

// All the mcufriend.com UNO shields have the same pinout.
// i.e. control pins A0-A4.  Data D2-D9.  microSD D10-D13.
// Touchscreens are normally A1, A2, D7, D6 but the order varies
//
// This should work with most Adafruit TFT libraries
// If you are not using a shield,  use a full Adafruit constructor()
// e.g. Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#include <SPI.h>          // f.k. for Arduino-1.5.2
#include "Adafruit_GFX.h"// Hardware-specific library
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* From this point, you could do anything you want */

#define DELAY 300

int has_sync = 0;
int has_data = 0;

/* draws sync circle (bottom left one) or clears it according to value */
void setSyncSignal(int value)
{
  if (value) tft.fillCircle(200, 40, 40, RED);
       else tft.fillCircle(200, 40, 40, BLACK);
}
/* draws data circle (center one) or clears it according to value */
void setDataSignal(int data)
{
  if (data) tft.fillCircle(120, 160, 40, RED);
       else tft.fillCircle(120, 160, 40, BLACK);
}

/* sends one bit */
void sendData(byte data)
{
  byte bits[9];
  bits[8] = 0; /* parity bit */
  Serial.print("'");
  Serial.write(data);
  Serial.print("' : ");
  Serial.print(data);
  Serial.print(" : ");
  for (int i = 7; i >= 0; i--)
  {
    bits[i] = data % 2;
    data = data / 2;
    bits[8] += bits[i]; 
    //Serial.print(bits[i]);
  }
  Serial.print("0b");

  bits[8] = (bits[8] % 2 == 0 ? 0 : 1);

  /* start bits */
  setSyncSignal(1);
  setDataSignal(0);
  delay(DELAY);
  setSyncSignal(0);
  delay(DELAY);
  setSyncSignal(0);
  setDataSignal(1);
  delay(DELAY);
  setDataSignal(0);
  delay(DELAY);
  
  /* data sending */
  for (int i = 0; i < 9; i++)
  {
     if (i == 8) Serial.print(" ");
     Serial.print(bits[i]);
     setSyncSignal(1);
     setDataSignal(bits[i]);
     delay(DELAY);
     setSyncSignal(0);
     setDataSignal(0);
     delay(DELAY);
  }
  Serial.println();
}

void setup(void) {
    Serial.begin(9600);
   
    tft.begin(0x1520);     //change this if display is not working correctly
    tft.fillScreen(BLACK); //init screen
    has_sync = 0;
    has_data = 0;
}

void loop(void) {
    unsigned long start;
    
    tft.fillCircle(40,40,40, BLUE);
    tft.fillCircle(40,280,40, BLUE);
    tft.fillCircle(200,280,40, BLUE);
    /*if (has_sync) 
    {
       tft.fillCircle(200,40,40, RED);
    }
     if (has_data) 
    {
       tft.fillCircle(120,160,40, RED);
    }*/
    if (Serial.available() > 0)
    {
      byte input;
      input = Serial.read();
      sendData(input);
      setSyncSignal(0);
      setDataSignal(0);
      /* tft.fillScreen(BLACK); */
    }
    
    delay(100);
}


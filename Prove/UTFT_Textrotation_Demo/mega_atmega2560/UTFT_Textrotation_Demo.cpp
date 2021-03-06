// UTFT_Textrotation_Demo 
// Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
// web: http://www.RinkyDinkElectronics.com/
//
// This program is a demo of the textrotation-functions.
//
// This demo was made for modules with a screen resolution 
// of 320x240 pixels.
//
// This program requires the UTFT library.
//

#include <UTFT.h>

// Declare which fonts we will be using
#include "arduino.h"

//
//
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Arduino Uno / 2009:
// -------------------
// Standard Arduino Uno/2009 shield            : <display model>,A5,A4,A3,A2
// DisplayModule Arduino Uno TFT shield        : <display model>,A5,A4,A3,A2
//
// Arduino Mega:
// -------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Mega      : <display model>,38,39,40,41
//
// Remember to change the model parameter to suit your display module!
#define TFTMODEL 1
// TENTATIVI FALLITI:	R61581,
UTFT myGLCD(TFTMODEL,38,39,40,41);

void setup()
{
	Serial.begin(115200);
	Serial.print("test TFT MODEL");	Serial.println(TFTMODEL);

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
}

void loop()
{
    myGLCD.print("Text rotation", 0, 0);
    myGLCD.setColor(0, 0, 255);
    myGLCD.print("0 degrees", 0, 16, 0);
    myGLCD.print("90 degrees", 319, 0, 90);
    myGLCD.print("180 degrees", 319, 239, 180);
    myGLCD.print("270 degrees", 0, 239, 270);

    myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.print("45", 90, 100, 45);
    myGLCD.print("90", 200, 50, 90);
    myGLCD.print("180", 300, 200, 180);

  while (true) {};
}




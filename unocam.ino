
//////////////////////////////////////////////////////////
/*
  Program: Quadcopter vision and data acquisition program
  Authors: Cole Panning and Ian Mackie
  Functions: Pixy camera, data comparasion, serial communications
  Version: who knows at this point
*/
//////////////////////////////////////////////////////////

//Note: remove all serial.prints before flight



#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Pixy.h>
#include <SoftwareSerial.h>
#include <Servo.h>


//////////////////////////////
//Pin definations
//////////////////////////////
#define shutterPin 4    //pin used to connect to ttl arduino
#define rollPin 9       //pin used to communicate with apm to move left and right
#define pitchPin 10     //pin used to communicate with the apm to move up and down
#define pwmPin 3       //pin used to change modes
#define selPin 8        //pin used to toggle outputs


/////////////////////////////
//Global Variables
/////////////////////////////
int var1, var2 = 0;             //Comparision flags for the quadcopter movement
Pixy pixy;                      //pixy class constructor
int xval = 0;                   //value to hold x capture data
int yval = 0;                   //value to hold y capture data
Servo roll;                     
Servo pitch;                    //Servo class items for pwm generation
Servo mode;
int selState = HIGH;
byte maxPulse = 150;
byte minPulse = 40;             //Variables for generating pwm waveform
byte restingPulse = 95;


/////////////////////////////
//Function Definations
/////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Strcompare function

void strcompare(int xcap, int ycap)          //function to compare strings for orientation
{
  //Note: 55 and 75 are test variables simulating the known center coordinates of the xy plane of the pxiy
  //      the 19 and 26 parts are actual sections from generated substrings that contain information from the
  //      naturally outputted pixy information about a capture, all without crazy string formatting, it is
  //      simply take what you need and use it
  //      Doing this way also allows us to compare the x and y coordinates individually one at a time making
  //      the control loops much easier to design and handle while using less space
  //Note: 319x199 is center pixel, 319 being on the x axis and 199 being on the y
  //      based on this, L/R movement should be done in relation to the 319, ex. 310-350
  //      and the U/D movement should be done in relation to the 199
  while((var1 != 1) && (var2 != 1))
  {
    //These lines represent movement along the x axis
    while(xcap > 350)  //Moves the quadcopter right towards center point
    {
      moveRight(10);                   //Move the quadcopter slowly to not accure momentum
      
    }
    while(xcap < 310)  //Move the quadcopter left towards center
    {
      moveLeft(25);                    //Move slowly 
    }
    if((xcap >= 310) && (xcap <= 350))    //Checks to see if quadcopter is withing parameters
    {
      var1 = 1;                        //Sets 2nd flag
    }
  //These lines represent movement along the y axis
    while(ycap > 210)  //Moves the quadcopter forward towards center
    {
      moveForward(10);                 //Move slowly
    }
    while(ycap < 188)  //Moves the quadcopter back towards center
    {
      moveBack(10);                    //Move slowly
    }
    //Note: this line does the same thing as the other 2 if flag statements,
    //      putting these like this is to test optimization
    if((ycap <= 210) && (ycap >= 188))
    {
      var2 = 1;                  //Set both flags if in correct margin of error
    }
  //Note:.startsWith("test variable", position paremeter) and .substring(position parameter)
  //     are both useful methods to analyize a string to test sections of it against known
  //     values, e.g. the center x and y coordinates of the pixy camera without crazy string
  //     formatting
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Movement functions

//Function to move quadcopter right
void moveRight(byte accel)    //variable accel needs to be a value 
{
  for(int h = 0; h < 1; h++)
  {
    roll.write(restingPulse + accel);
    delay(500);
  }
  roll.write(restingPulse + 43);
}

void moveLeft(byte accel)    //Move the quadcopter left
{
  for(int i = 0; i < 1; i++)
  {
    roll.write(restingPulse - accel);
    delay(500);
  }
  roll.write(restingPulse - 3);          //value to generate neutral thrust
}

void moveForward(byte accel) //Move the quadcopter forward
{
  for(int q = 0; q < 1; q ++)
  {
    pitch.write(restingPulse - accel);
    delay(500);
  }
  pitch.write(restingPulse - 3);
}

void moveBack(byte accel)    //Move the quadcopter back
{
  for(int m = 0; m < 1; m++)
  {
    pitch.write(restingPulse + accel);
    delay(500);
  }
  pitch.write(restingPulse + 43);
}
//Note: using the yaw controls would allow the quadcopter to rotate CW or CCW
//      and might be more efficient for position than moving the whole quadcopter a bunch of times
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Mode change function

void modeChange(byte accel)                //variable accel is needs to be a value between 6 and 43
{
  mode.write(restingPulse - accel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Setup loop to define pins and initialize key elements
void setup() 
{
  Serial.begin(9600);                        //Starts serial connection, 9600 baud is standard
  pixy.init();                               //Initialize the pixy camera  Note: still works after being commented out
  pinMode(selPin, OUTPUT);                   //Sets the mode select pin to output
  pinMode(shutterPin, OUTPUT);               //Sets shutter pin as an output to connect to ttluno
  digitalWrite(shutterPin, HIGH);            //Use internal pullup resistor for triggering
  digitalWrite(selPin, HIGH);
  roll.attach(rollPin);                      //Attach pin to servo class object for roll control
  pitch.attach(pitchPin);                    //Attach pin to servo calss object for pitch control
  mode.attach(pwmPin);                       //Attach pin to servo class object for autopilot mode change
}                                            //End setup, begin main loop

//Main loop (repeats infinitely)
void loop() 
{
  
  //pixy.init();        //Note: moving init here instead of setup causes many no detections and a single detection per loop iteration
  static int i = 0;   //Frame counting variable
  int j;              //Counting variable used in for loop
  uint16_t blocks;    //variable to hold captured block data
  char buf[32];       //buffer to hold data for x axis capture
  char buf2[32];      //buffer to hold data for y axis capture
  
  
  blocks = pixy.getBlocks();    //Capture block data
  
  if (blocks)                   //If block data is captured, print it
  {
    i++;
    //Serial.print("detections\n");
    // do this (print) every 50 frames because printing every
    // frame would bog down the Arduino
    if (i%50==0)
    {
      for (j=0; j<blocks; j++)
      {
        //Note: a nested for loop might pass y value easier than the method below
        sprintf(buf, "%d", pixy.blocks[j].x);    //Copy block data into buffer for processing
        sprintf(buf2, "%d", pixy.blocks[j].y);   //Copy block data into buffer2 for processing
      }
      xval = atoi(buf);                          //Converts char array buf into an int for passing to function
      yval = atoi(buf2);                         //Converts char array buf2 into an int for passing to function
    }
  }
  if((xval > 0) && (yval > 0))                  //function to attempt to prevent false positives
  {
    digitalWrite(shutterPin, LOW);
    digitalWrite(selPin, LOW);
    //delay(500);
    modeChange(6);                              //Switch mode to Althold mode if no false positives
    digitalWrite(shutterPin, HIGH);             //trigger ttl uno to take picture
    modeChange(43);                             //Return to auto mode
    //Serial.print("Full manual mode\n");
    digitalWrite(selPin, HIGH);                  //Disable arduino control, enable TX control
    delay(3500);                                 //Wait for picture to be saved to SD card
    var1, var2 = 0;  //Resets flag variables for next use
    xval, yval = 0;
  } 
 digitalWrite(shutterPin, LOW);                  //resets trigger pin for next picture 
}


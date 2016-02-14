#include <Arduino.h>
#include <Adafruit_VC0706.h>
//Use tiny gps library
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>         
      
// SD card chip select line varies among boards/shields:
// Adafruit SD shields and modules: pin 10
// Arduino Ethernet shield: pin 4
// Sparkfun SD shield: pin 8
// Arduino Mega w/hardware SPI: pin 53
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20

// Pins for camera connection are configurable.
// With the Arduino Uno, etc., most pins can be used, except for
// those already in use for the SD card (10 through 13 plus
// chipSelect, if other than pin 10).
// With the Arduino Mega, the choices are a bit more involved:
// 1) You can still use SoftwareSerial and connect the camera to
//    a variety of pins...BUT the selection is limited.  The TX
//    pin from the camera (RX on the Arduino, and the first
//    argument to SoftwareSerial()) MUST be one of: 62, 63, 64,
//    65, 66, 67, 68, or 69.  If MEGA_SOFT_SPI is set (and using
//    a conventional Arduino SD shield), pins 50, 51, 52 and 53
//    are also available.  The RX pin from the camera (TX on
//    Arduino, second argument to SoftwareSerial()) can be any
//    pin, again excepting those used by the SD card.
// 2) You can use any of the additional three hardware UARTs on
//    the Mega board (labeled as RX1/TX1, RX2/TX2, RX3,TX3),
//    but must specifically use the two pins defined by that
//    UART; they are not configurable.  In this case, pass the
//    desired Serial object (rather than a SoftwareSerial
//    object) to the VC0706 constructor.

// On Uno: camera TX connected to pin 2, camera RX to pin 3:
// On Mega: camera TX connected to pin 69 (A15), camera RX to pin 3:
//SoftwareSerial cameraconnection = SoftwareSerial(69, 3);

//////////////////////////////////////////////////////////////////////////////////////////////
//Assign Serial connection pins
SoftwareSerial cameraconnection = SoftwareSerial(5, 4);    //Serial connection for TTL camera
//SoftwareSerial mySerial(8, 7);                             //Serial connection for GPS

//////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////
//Attach serial pins to class object
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);

//////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////
//Pin and register Definations
#define chipSelect 10    //CS pin used to communicate with the SD card Note: check for redundant code
#define ledPin 13        //Status light pin defination
#define shutterPin 2     //Pin designated to house interrupt
//////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////
//Setup Loop
void setup() 
{
  Serial.begin(9600);                           //Starts general serial connection
  cameraconnection.begin(9600);                 //Starts serial for camera, standard baud, maximum 115200
  pinMode(shutterPin, INPUT);                   //Sets input trigger pin as input to wait for signal
  digitalWrite(shutterpin, LOW);                //Hold shutter pin low and wait for high to take picture
  //pinMode(ledPin, OUTPUT);                      //Sets the status led pin as an output, removed with gps stuff
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
   
  //Serial.println("Starting TTL camera");
  // Try to locate the camera, not really necessary code
  if (cam.begin()) 
  {
    Serial.println("Camera Found:");
  } 
  else 
  {
    Serial.println("No camera found?");
    return;
  }
  if (!SD.begin(chipSelect)) 
    {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      return;
    }  
  cam.setImageSize(VC0706_640x480);        //Largest picture size camera can take
}    //End setup, begin main loop

//Main loop (repeats infinitely)
void loop() 
{
  //polls the snapvar variable until true then takes a picture
  if(digitalRead(shutterPin) == HIGH)
  {
    if (! cam.takePicture())
    { 
      Serial.println("Failed to snap!");
      return;
    }
    else 
      Serial.println("Picture taken!");
  
    // Create an image with the name IMAGExx.JPG
    char filename[13];
    strcpy(filename, "IMAGE00.JPEG");
    for (int i = 0; i < 100; i++) 
    {
      filename[5] = '0' + i/10;
      filename[6] = '0' + i%10;
      // create if does not exist, do not open existing, write, sync after write
      if (! SD.exists(filename)) 
      {
        break;
      }
    }
  
    // Open the file for writing
    File imgFile = SD.open(filename, FILE_WRITE);

    // Get the size of the image (frame) taken  
    uint16_t jpglen = cam.frameLength();
    Serial.print("Storing ");
    Serial.print(jpglen, DEC);
    Serial.print(" byte image.");

    int32_t time = millis();
    pinMode(8, OUTPUT);
    // Read all the data up to # bytes!
    byte wCount = 0; // For counting # of writes
    while (jpglen > 0) 
    {
      // read 32 bytes at a time;
      uint8_t *buffer;
      uint8_t bytesToRead = min(64, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
      buffer = cam.readPicture(bytesToRead);
      imgFile.write(buffer, bytesToRead);
      if(++wCount >= 64) 
      {
        //Every 2K, give a little feedback so it doesn't appear locked up
        Serial.print('.');
        wCount = 0;
      }
      //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
      jpglen -= bytesToRead;
    }
    imgFile.close();

    time = millis() - time;
    Serial.println("done!");
    Serial.print(time); 
    Serial.println(" ms elapsed");
    resetFunc();
  }
}

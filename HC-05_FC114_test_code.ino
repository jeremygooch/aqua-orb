/*
  The following Code was lifted from: http://acoptex.com/project/151/basics-project-028a-arduino-bluetooth-modules-hc-05-and-hc-06-zs-40-fc-114-at-acoptexcom/#sthash.99AO5bgG.lfRZlerm.dpbs

  Project: Bluetooth HC-05 ZS-40 TEST using software serial library
  Function: Uses hardware serial to talk to the host computer and software serial
          for communication with the Bluetooth module. Intended for Bluetooth
          devices that require line end characters "\r\n". When a command is entered
          in the serial monitor on the computer the Arduino will relay it to the
          bluetooth module and display the result.
  Pins
  Arduino  - Bluetooth HC-05 ZS-40
  5V pin     VCC pin
  GND pin    GND pin
  Digital 3  RX pin through a voltage divider
  Digital 2  TX pin (no need voltage divider)

*/
//*********************************************************************************
#include <SoftwareSerial.h>//include library code
//*********************************************************************************
SoftwareSerial BTserial(2, 3); // RX | TX
// Connect the HC-05 TX to the Arduino RX on pin 2. 
// Connect the HC-05 RX to the Arduino TX on pin 3 through a voltage divider.
//*********************************************************************************
void setup()
{
  Serial.begin(9600);//initialize serial communication at 9600 bps
  BTserial.begin(38400);  //It can also be configured to have different baud rates:9600,
//19200,38400 etc. By default the baud rate is 38400. If 38400 doesn't work try different baud rate
}
void loop()
{
  // Read from the Bluetooth module and send to the Arduino Serial Monitor
  if (BTserial.available())
  {   
    Serial.write(BTserial.read());
  }
  // Read from the Arduino Serial Monitor and send to the Bluetooth module
  if (Serial.available())
  {    
    BTserial.write(Serial.read());    
    }
  }

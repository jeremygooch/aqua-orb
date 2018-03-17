// NOTE
// Hold down the reset button when sending commands in AT mode
// AT+PSWD="0000"
// AT+NAME="my-name"
//*********************************************************************************
#include <SoftwareSerial.h>//include library code
//*********************************************************************************
SoftwareSerial BTserial(8,9); // RX | TX
// Connect the HC-05 TX to the Arduino RX on pin 2. 
// Connect the HC-05 RX to the Arduino TX on pin 3 through a voltage divider.
//*********************************************************************************
void setup()
{
    Serial.begin(9600);//initialize serial communication at 9600 bps
    BTserial.begin(38400);  //It can also be configured to have different baud rates:9600,
    //19200,38400 etc. By default the baud rate is 38400. If 38400 doesn't work try different baud rate
    Serial.println("Initting");
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

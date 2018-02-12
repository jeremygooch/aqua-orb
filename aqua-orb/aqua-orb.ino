#include <SoftwareSerial.h>
#include <Servo.h>
#include <TimerObject.h>
#include <PCD8544.h>

SoftwareSerial BTserial(8, 9); // RX | TX

TimerObject *timer1 = new TimerObject(1000); // Main loop timer

Servo myservo;  // create servo object to control a servo

boolean debug = true;


boolean singleClose = false;
int singleCloseCounter = 5;

const byte numChars = 60;
const char defaultOpts[] = "f=0003600-t=0002-o=000-c=100-n=Pilea AluminumPlant";
char receivedChars[numChars];
boolean newData = false;

static long offTime = 32;
static long openTime = 23;
String prevTime;
int servoOpen = 0;
int servoClose = 0;
char label[25];

boolean systemTestDone = false;
boolean interruptCMD = false;

int servoFlag = 0;


#define pinServo A0

static PCD8544 lcd;

void setup()
{
    // For Debugging
    Serial.begin(9600);

    lcd.begin(84, 48);

    // HC-05 default serial speed for commincation mode is 9600
    BTserial.begin(9600);

    pinMode(LED_BUILTIN, OUTPUT);

    myservo.attach(pinServo);  // attaches the servo to the servo object
    buildOptions(0);
    Serial.println(servoClose);
    /* Global Options are: */
    /* Serial.println(offTime); */
    /* Serial.println(openTime); */
    /* Serial.println(servoClose); */
    /* Serial.println(servoOpen); */
    /* Serial.println(label); */

    timer1->setOnTimer(&mainWaterLoop);
    timer1->Start();
}

void loop()
{
    digitalWrite(LED_BUILTIN, LOW);

    timer1->Update();
}

void btLoop() {
    if (BTserial.available() > 0) {
        recvWithMarkers();
    }
    if (newData) {
        /* timer1->Stop(); */
        parseData();
        /* timer1->setOnTimer(&btLoop); */
        /* timer1->Start(); */
    }
}

void mainWaterLoop(boolean updateVars, long ofTim, long onTim, int svOp, int svCl) {
    if (updateVars == true) {
        offTime = ofTim;
        openTime = onTim;
        servoOpen = svOp;
        servoClose = svCl;
    }

    static int counterSysTest = 2;
    static int closeCounterSysTest = 1;
    static long waterInterval = offTime;
    static long waterOpenTime = openTime;
    String remainingTime;

    if (systemTestDone == false) {
        lcd.setCursor(0, 0);
        if (counterSysTest > 0) {
            lcd.print(String("SYS CHECK IN: ") + String(counterSysTest) + String("s"));
            counterSysTest--;
        } else if (counterSysTest == 0) {
            lcd.setCursor(0, 0);
            lcd.print("TESTING SYSTEM                   ");
            toggleServo(true);
            if (closeCounterSysTest > 0) {
                closeCounterSysTest--;
            } else {
                lcd.setCursor(0, 0);
                lcd.print("TESTS COMPLETE");
                toggleServo(false);
                systemTestDone = true;
            }
        }
    }

    if (systemTestDone && interruptCMD == false) {
        btLoop();
        /* Serial.println("waterInterval"); */
        /* Serial.println(waterInterval); */
        if (waterInterval > 0) {
            // Only update lcd if the time has changed
            remainingTime= humanReadableTime(waterInterval);
            if (remainingTime != prevTime) {
                lcd.setCursor(0, 0);
                lcd.print("Watering in:  ");
                lcd.setCursor(0, 1);
                lcd.print(String(remainingTime) + String("     "));
                lcd.setCursor(0, 3);
                lcd.print(String(label));
                waterInterval--;
            }
        } else {
            toggleServo(true);
            if (waterOpenTime > 0) {
                remainingTime= humanReadableTime(waterOpenTime);
                lcd.setCursor(0, 0);
                lcd.print("Watering.......");
                lcd.setCursor(0, 1);
                lcd.print(String(remainingTime) + String("     "));
                lcd.setCursor(0, 3);
                lcd.print(String(label));
                waterOpenTime--;
            } else {
                toggleServo(false);
                waterOpenTime = openTime;
                waterInterval = offTime;
            }
        }
    }

    if (singleClose) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WATER CMD RCVD");
        toggleServo(true);
        if (singleCloseCounter > 0) {
            remainingTime = humanReadableTime(singleCloseCounter);
            lcd.setCursor(0, 1);
            lcd.print("Closing in: ");
            lcd.setCursor(0, 2);
            lcd.print(String(remainingTime) + String("     "));
            interruptCMD = true;
            singleCloseCounter--;
        } else {
            lcd.clear();
            toggleServo(false);
            singleCloseCounter = 5;
            interruptCMD = false;
            singleClose = false;
        }
    }
}

String humanReadableTime(double x) {
    String out;
    String plural;
    long v = 0;
    if (x >= 86400) {
        v = x / 86400;
        if (x >= 172800) { plural = "s"; }
        else { plural = ""; }
        out = (int)(round(v)) + String("day") + plural;
    } else if (x < 86400 && x >= 3600) {
        v = x / 3600;
        if (x >= 7200) {
            out = (int)(round(v)) + String("-ish hrs") + plural;
        } else {
            out = String("about an hour or so");
        }
    } else if (x < 3600 && x >= 60) {
        v = x / 60;
        if (x == 3599) { lcd.clear(); }
        if (x >= 120) { plural = "s"; }
        else { plural = ""; }
        out = (int)(round(v)) + String("-ish min") + plural;
    } else if (x < 60) {
        out = (int)x + String("sec       ");
    }
    return out;
}

void toggleServo(boolean open) {
    if (open == true) {
        if (myservo.read() != servoOpen) {
            myservo.write(servoOpen);
        }
    } else {
        if (myservo.read() != servoClose) {
            myservo.write(servoClose);
        }
    }
}


void parseData()
{
    newData = false;
    String errOut('Invalid option specified.');
    if (receivedChars[0] == 't') {
        if (interruptCMD == false) { // make sure nothing odd is running
            // Temporarily turn the servo for a set amount of time
            int len = strlen(receivedChars);
            char *digitsOnly = receivedChars + (len < 4 ? 0 : len - 4);
            singleCloseCounter = atol(digitsOnly);

            buildOptions(0);
            singleClose = true;
        }
    } else if (receivedChars[0] == 'f') {
        // Set the frequency and options
        timer1->Stop();
        buildOptions(1);

        mainWaterLoop(true, offTime, openTime, servoOpen, servoClose);
        timer1->setOnTimer(&mainWaterLoop);
        timer1->Start();
    } else if (receivedChars[0] == 'q') {
        String offTimeV;
        String labelV;
        String openTimeV;
        String servoOpenV;
        String servoCloseV;
        String sep;
        String term;
        String beginning;
        String quote;
        String f;
        String n;
        String t;
        String o;
        String c;

        offTimeV = offTime;
        labelV = label;
        openTimeV = openTime;
        servoOpenV = servoOpen;
        servoCloseV = servoClose;
        beginning = '{';
        sep = ',';
        term = ':';
        quote = '"';
        f="frequency";
        n="name";
        t="timeOpen";
        o="servoOpen";
        c="servoClose";

        BTserial.print(beginning +
                       quote + f + quote + term + offTimeV + sep +
                       quote + n + quote + term + quote + labelV + quote + sep +
                       quote + t + quote + term + openTimeV + sep +
                       quote + o + quote + term + servoOpenV + sep +
                       quote + c + quote + term + servoCloseV + '}');


        // BACKWARDS SUPPORT for connecting via device list
        if (receivedChars[1] == 'n') { // name/ plant label
            BTserial.print('{' + label + '}');
        }
        

        /* String out; */
        /* String prop; */
        /* if (receivedChars[1] == 'f') { // frequency open */
        /*     out = offTime; */
        /*     prop = 'f'; */
        /* } else if (receivedChars[1] == 'n') { // name/ plant label */
        /*     out = label; */
        /*     prop = 'n'; */
        /* } else if (receivedChars[1] == 't') { // time open */
        /*     out = openTime; */
        /*     prop = 't'; */
        /* } else if (receivedChars[1] == 'o') { // open value */
        /*     out = servoOpen; */
        /*     prop = 'o'; */
        /* } else if (receivedChars[1] == 'c') { // closed value */
        /*     out = servoClose; */
        /*     prop = 'c'; */
        /* } else { */
        /*     out = errOut; */
        /*     prop = 'e'; */
        /* } */
        /* out = '[' + out + ']'; */
        /* BTserial.print(prop + out); */
    } else {
        BTserial.print('[' + errOut + ']');
    }
}

void buildOptions(int optType)
{
    // Current Options format
    /* f:0000060|t:0005|o:000|c:100|n:asdfghjkl-wertyui-pzxcvbn */
    char opts[56];
    if (optType == 0) {
        strcpy (opts, defaultOpts);
    } else {
        strcpy (opts, receivedChars);
    }

    char offTimeArr[7];
    strncpy (offTimeArr, opts + 2, 7);
    offTimeArr[7] = '\0';
    offTime = atol(offTimeArr);

    char onTimeArr[4];
    strncpy (onTimeArr, opts + 12, 4);
    onTimeArr[4] = '\0';
    openTime = atol (onTimeArr);

    char servoOpenArr[3];
    strncpy (servoOpenArr, opts + 19, 3);
    servoOpenArr[3] = '\0';
    servoOpen = atol(servoOpenArr);

    char servoCloseArr[3];
    strncpy (servoCloseArr, opts + 25, 3);
    servoCloseArr[3] = '\0';
    servoClose = atol(servoCloseArr);
    Serial.write("calculating servoClosed...");
    Serial.println(servoClose);

    strncpy (label, opts + 31, 25);
    label[25] = '\0';
}

void recvWithMarkers()
{
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '[';
    char endMarker = ']';
    char rc;

    if (BTserial.available() > 0) {
        rc = BTserial.read();
        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) { ndx = numChars - 1; }
            } else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        } else if (rc == startMarker) { recvInProgress = true; }
    }
}

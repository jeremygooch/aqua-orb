#include <SoftwareSerial.h>
#include <Servo.h>
#include <TimerObject.h>
#include <PCD8544.h>

SoftwareSerial BTserial(8, 9); // RX | TX

TimerObject *timer1 = new TimerObject(1000); // Main loop timer

Servo myservo;  // create servo object to control a servo

boolean debug = true;

const byte numChars = 60;
const char defaultOpts[] = "f=0000020-t=0002-o=000-c=100-n=Pilea AluminumPlant";
char receivedChars[numChars];
boolean newData = false;

static long offTime = 32;
static long openTime = 23;
static long offTimeBT = 32;
static long openTimeBT = 23;
static long servoOpen = 0;
static long servoClose = 0;


int servoOpenBT = 50;
static long servoCloseBT = 0;
String prevTime;
char label[25];
char labelBT[25];

boolean systemTestDone = false;
boolean interruptCMD = false;
boolean parsingBTData = false;
boolean updateParamsFromBT = false;

// LCD Globals
String closeMsg("Watering in:  ");
String openMsg("Watering for:  ");
String timeLeft;
String note;


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
    buildOptions();
    Serial.println(servoClose);

    timer1->setOnTimer(&mainWaterLoop);
    timer1->Start();
}

void loop()
{
    btLoop();
    timer1->Update();
}

void btLoop() {
    if (BTserial.available() > 0) {
        myservo.detach();
        recvWithMarkers();
    }
    if (newData) {
        parseData();
        myservo.attach(pinServo);
    }
}

void mainWaterLoop() {
    if (parsingBTData == false) {
        if (updateParamsFromBT) {
            offTime = offTimeBT;
            strncpy(label, labelBT, 25);
            openTime = openTimeBT;
            Serial.println("after?");
            Serial.println(servoOpenBT);
            servoOpen = servoOpenBT;
            /* servoClose = servoCloseBT; */
            lcd.clear();
            updateParamsFromBT = false;
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
                    lcd.clear();
                    toggleServo(false);
                    systemTestDone = true;
                }
            }
        }

        if (systemTestDone && interruptCMD == false) {
            if (waterInterval > 0) {
                remainingTime= humanReadableTime(waterInterval);
                if (remainingTime != prevTime) {
                    timeLeft = remainingTime;
                    updateLCD(closeMsg);
                    waterInterval--;
                }
            } else {
                toggleServo(true);
                if (waterOpenTime > 0) {
                    remainingTime= humanReadableTime(waterOpenTime);
                    // To optimize.. remove clears and combine common renders
                    timeLeft = remainingTime;
                    updateLCD(openMsg);
                    waterOpenTime--;
                } else {
                    lcd.clear();
                    toggleServo(false);
                    waterOpenTime = openTime;
                    waterInterval = offTime;
                    note = "";
                }
            }
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

void updateLCD(String lcdmsg) {
    lcd.setCursor(0, 0);
    lcd.print(lcdmsg);
    lcd.setCursor(0, 1);
    lcd.print(String(timeLeft) + String("               "));
    lcd.setCursor(0, 3);
    lcd.print(String(label) + String(note));
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

    parsingBTData = true;

    if (receivedChars[0] == 'f') {
        char optsX[56];
        strcpy (optsX, receivedChars);

        char offTimeArrX[7];
        strncpy (offTimeArrX, optsX + 2, 7);
        offTimeArrX[7] = '\0';
        offTimeBT = atol(offTimeArrX);

        char onTimeArrX[4];
        strncpy (onTimeArrX, optsX + 12, 4);
        onTimeArrX[4] = '\0';
        openTimeBT = atol (onTimeArrX);

        char servoOpenArrX[3];
        strncpy (servoOpenArrX, optsX + 19, 3);
        servoOpenArrX[3] = '\0';
        Serial.println("b4 processing");
        Serial.println(servoOpenArrX);
        servoOpenBT = atol(servoOpenArrX);

        Serial.println("servo open will now be");
        Serial.println(servoOpenBT);

        char servoCloseArrX[3];
        strncpy (servoCloseArrX, optsX + 25, 3);
        servoCloseArrX[3] = '\0';
        servoCloseBT = atol(servoCloseArrX);

        strncpy (labelBT, optsX + 31, 25);
        labelBT[25] = '\0';

        updateParamsFromBT = true;
        note = "*";

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
    } else {
        BTserial.println('[' + errOut + ']');
    }
    parsingBTData = false;
}

void buildOptions()
{
    // Current Options format
    /* f:0000060|t:0005|o:000|c:100|n:asdfghjkl-wertyui-pzxcvbn */
    char opts[56];
    strcpy (opts, defaultOpts);

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
        } else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

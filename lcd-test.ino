#include <PCD8544.h>

static PCD8544 lcd;

void setup()
{
    // For Debugging
    Serial.begin(9600);

    lcd.begin(84, 48);

    lcd.clear();
}


void loop()
{
    lcd.setCursor(0, 0);
    lcd.print("asdf asdf asdf");
}
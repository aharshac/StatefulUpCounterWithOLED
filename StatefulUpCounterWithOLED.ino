// Library imports
#include <Bounce2.h>  // Contact bouncing 
#include <EEPROM.h>   

#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>


/* Constants and variables */
const PROGMEM unsigned int MAX_RAM = 2048; // Bytes


// Line separator for Serial console
const PROGMEM char SEPARATOR[] = "-------------------------------------";


// OLED rows and columns
const PROGMEM unsigned int TITLE_ROW = 0;     // pixels/8
const PROGMEM unsigned int TITLE_COL = 0;     // pixels
const PROGMEM unsigned int SUBTITLE_ROW = 1;
const PROGMEM unsigned int SUBTITLE_COL = 0;
const PROGMEM unsigned int COUNT_ROW = 2;
const PROGMEM unsigned int PLUS_COL = 0;
const PROGMEM unsigned int COUNT_COL = 60;
const PROGMEM unsigned int RESET_COL = 37;


// Used GPIO pins
const PROGMEM unsigned int pinUp = 9;
const PROGMEM unsigned int pinReset = 2;
const PROGMEM unsigned int pinLed = 13;


// Button Objects
Bounce buttonUp = Bounce(); 
Bounce buttonReset = Bounce(); 


// OLED
SSD1306AsciiAvrI2c oled;

// Statefulness
bool stateLed = false;
bool prevStateUp = false;
bool prevStateReset = false;

struct Counter {
  unsigned int count = 0;
} counter;



// Device management
void(*ResetNow) (void) = 0;


unsigned int GetFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


// Edge case handling
void CheckAndLimitCounter() {
  if (counter.count < 0) counter.count = 0;
  if (counter.count > 50000) counter.count = 0;
}


// Output
void LED_Toggle() {
  stateLed = !stateLed;
  digitalWrite(pinLed, stateLed ? HIGH : LOW);
}


void Serial_PrintSeparator() {
  Serial.println((__FlashStringHelper*)SEPARATOR);
}

void Display_ClearLine(int row, int startCol = 0, bool size2x = false) {
  if (!size2x) 
    oled.set1X();
  else
    oled.set2X();
  oled.setCursor(startCol, row);
  oled.clearToEOL();
  delay(5);
}

void Display_SplashScreen() {
  Serial.println(F(""));
  Serial.println(F("Stateful Up Counter With OLED Display"));
  
  oled.set1X();
  oled.setCursor(TITLE_COL, TITLE_ROW);
  oled.println("COLLABORIZM");
  delay(5);

  oled.setCursor(SUBTITLE_COL, SUBTITLE_ROW);
  oled.println("STATEFUL");
  delay(5);

  oled.set2X();
  oled.setCursor(0, COUNT_ROW);
  oled.println("UP COUNTER");
}


void Display_FreeRam() {
  unsigned int freePercent = (GetFreeRam() / (float)MAX_RAM) * 100;

  char buffer[19];
  sprintf(buffer, "Free Memory = %d%%", freePercent);

  Serial.println(buffer);
  Display_ClearLine(SUBTITLE_ROW);

  oled.set1X();
  oled.setCursor(SUBTITLE_COL, SUBTITLE_ROW);
  oled.println(buffer);
  delay(5);
}


void Display_Count() {
  char buffer[6];
  sprintf(buffer, "%5d", counter.count);

  Display_ClearLine(COUNT_ROW, COUNT_COL, true);
  
  oled.set2X();
  oled.setCursor(COUNT_COL, COUNT_ROW);
  oled.print(buffer);
  delay(5);
}


void Display_PlusSign() { 
  oled.set2X();
  oled.setCursor(PLUS_COL, COUNT_ROW);
  oled.clear(PLUS_COL, COUNT_COL - 1, COUNT_ROW, COUNT_ROW + 1);
  delay(5);

  oled.set2X();
  oled.print("+");
  delay(100);
  
  oled.set2X();
  oled.setCursor(PLUS_COL, COUNT_ROW);
  oled.clear(PLUS_COL, COUNT_COL - 1, COUNT_ROW, COUNT_ROW + 1);
  delay(50);
}


void Display_Reset() {
  Display_ClearLine(SUBTITLE_ROW);
  Display_ClearLine(COUNT_ROW, COUNT_COL, true);
   
  oled.set2X();
  oled.setCursor(RESET_COL, COUNT_ROW);
  oled.println("RESET");
  delay(50);
}

void setup() {
  Serial.begin(9600);
  
  oled.begin(&Adafruit128x32, 0x3C);
  oled.setFont(Adafruit5x7);  

  Display_SplashScreen();

  Serial.println(F("Initializing Hardware"));
  pinMode(pinLed, OUTPUT);
  pinMode(pinUp, INPUT_PULLUP);
  pinMode(pinReset, INPUT_PULLUP);

  buttonUp.attach(pinUp);
  buttonUp.interval(5);
  buttonReset.attach(pinReset);
  buttonReset.interval(5);

  Serial.println(F("Reading EEPROM"));
  EEPROM.get(0, counter);
  CheckAndLimitCounter();
  
  Serial.print(F("Stored count = "));
  Serial.println(counter.count);

  delay(1500);
  Display_FreeRam();

  Display_ClearLine(COUNT_ROW, 0, true);
  Display_Count();

  Serial.println(F("Ready..."));
  Serial_PrintSeparator();
}
 
 
void loop() {
  buttonUp.update();
  buttonReset.update();

  bool pressUp = (buttonUp.read() == LOW);
  bool pressReset = (buttonReset.read() == LOW);

  if (prevStateUp == pressUp && prevStateReset == pressReset) {
    return;
  } else {
    LED_Toggle();
  }
        
  prevStateUp = pressUp;
  prevStateReset = pressReset;

  if (pressUp && !pressReset) {
    Serial.println(F("Up button pressed."));

    Serial.print(F("Incrementing count to "));
    CheckAndLimitCounter();
    counter.count += 1;
    Serial.println(counter.count);

    Display_PlusSign();
    Display_Count();
    
    Serial.println(F("Saving to EEPROM"));
    EEPROM.put(0, counter);

    Serial_PrintSeparator();
    delay(300);
    return;
  }

  if (pressReset && !pressUp) {
    Serial.println(F("Reset button pressed."));

    Display_Reset();
    
    Serial.println(F("Clearing EEPROM"));
    counter.count = 0;
    EEPROM.put(0, counter);
    
    Serial.println(F("Restarting..."));
    delay(1000);
    
    oled.clear();
    ResetNow();
  }
}

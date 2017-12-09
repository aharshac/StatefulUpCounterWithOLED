// Library imports
#include <Bounce2.h>  // Contact bouncing 
#include <EEPROM.h>   
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



/* Constants and variables */
const int OLED_RESET = 4;

const int MAX_RAM = 2048; // Bytes
const PROGMEM char STR_FREE_RAM[] = "Free Memory = %d %%";

const PROGMEM char SEPARATOR[] = "-------------------------------------";   // Line separator for Serial console

// Clear OLED line without clearing whole screen
const PROGMEM char CLEAR_FONT_SIZE_1[] = "                             ";   // 128/6 = 29 whitespace characters
const PROGMEM char CLEAR_FONT_SIZE_2[] = "          ";   // 128/12 = 10 whitespace characters



// Used GPIO pins
const int pinUp = 9;
const int pinReset = 2;
const int pinLed = 13;



// Button Objects
Bounce buttonUp = Bounce(); 
Bounce buttonReset = Bounce(); 
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
  #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif



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
void ToggleLed() {
  stateLed = !stateLed;
  digitalWrite(pinLed, stateLed ? HIGH : LOW);
}


void SerialPrintSeparator() {
  Serial.println((__FlashStringHelper*)SEPARATOR);
}

void ClearLine(int y = 0, int fontSize = 1) {
  display.setTextSize(fontSize == 2 ? 2 : 1);
  display.setTextColor(0xFFFF, 0);
  display.setCursor(0, y);
  display.print("      ");
  display.display();
  delay(5);
}

void DisplaySplashScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("COLLABORIZM");
  display.display();
  delay(5);

  display.setTextSize(1);
  display.setCursor(0,8);
  display.println("STATEFUL");
  display.display();
  delay(5);

  display.setTextSize(2);
  display.setCursor(0,18);
  display.println("UP COUNTER");
  display.display();
  delay(2000);
}


void DisplayFreeRam() {
  unsigned int freePercent = (GetFreeRam() / MAX_RAM) * 100;

  //char buffer[strlen(STR_FREE_RAM) + 3];
  //sprintf(buffer, STR_FREE_RAM, freePercent);
  
  ClearLine(0, 1);

  /*
  display.setCursor(0,0);
  display.println(buffer);
  display.display();
  delay(5);*/
}

void setup() {
  Serial.begin(9600);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();

  Serial.println(F(""));
  Serial.println(F("Stateful Up Counter With OLED Display"));
  
  DisplaySplashScreen();
  
  Serial.print(F("Free RAM (bytes) = "));
  Serial.println(GetFreeRam());

  DisplayFreeRam();

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

  Serial.println(F("Ready..."));
  SerialPrintSeparator();
}
 
 
void loop() {
  buttonUp.update();
  buttonReset.update();

  bool pressUp = (buttonUp.read() == LOW);
  bool pressReset = (buttonReset.read() == LOW);

  if (prevStateUp == pressUp && prevStateReset == pressReset) {
    return;
  } else {
    ToggleLed();
  }
        
  prevStateUp = pressUp;
  prevStateReset = pressReset;

  if (pressUp && !pressReset) {
    Serial.println(F("Up button pressed."));

    Serial.print(F("Incrementing count to "));
    CheckAndLimitCounter();
    counter.count += 1;
    Serial.println(counter.count);
    
    Serial.println(F("Saving to EEPROM"));
    EEPROM.put(0, counter);

    SerialPrintSeparator();
    delay(500);
    return;
  }

  if (pressReset && !pressUp) {
    Serial.println(F("Reset button pressed."));

    Serial.println(F("Clearing EEPROM"));
    counter.count = 0;
    EEPROM.put(0, counter);
    
    Serial.println(F("Restarting..."));
    delay(1000);
    ResetNow();
  }
}

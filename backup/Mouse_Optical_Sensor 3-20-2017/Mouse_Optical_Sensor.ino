// Libraries ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <A3000.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined(ARDUINO_ARCH_SAMD)
  #include <SoftwareSerial.h>
#endif
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>
#include "BluefruitConfig.h"

// Definitions /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MOUSE_WHEEL_A             3  // interrupt pin - DO NOT CHANGE
#define MOUSE_WHEEL_B             7  // interrupt pin - DO NOT CHANGE
#define L_BUTTON                  10
#define M_BUTTON                  11
#define R_BUTTON                  2
#define SCROLL_FACTOR             1

#define FACTORYRESET_ENABLE       0
#define MINIMUM_FIRMWARE_VERSION  "0.6.6"

// Global Variables ////////////////////////////////////////////////////////////////////////////////////////////////////////
A3000 Sensor;
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

volatile int counter = 0;
volatile byte lastState = 0;
volatile byte currentState = 0;
bool leftIsPressed = false;
bool rightIsPressed = false;
bool middleIsPressed = false;
enum mouseButton {LEFT, RIGHT, MIDDLE};

// Setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(L_BUTTON, INPUT);
  pinMode(R_BUTTON, INPUT);
  pinMode(M_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(MOUSE_WHEEL_A), mouseWheelTurn, CHANGE);
  attachInterrupt(digitalPinToInterrupt(MOUSE_WHEEL_B), mouseWheelTurn, CHANGE);
  Sensor.init();
  Serial.begin(115200);

  if (!ble.begin(VERBOSE_MODE)) { error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?")); }
  Serial.println(F("OK!"));

  if (FACTORYRESET_ENABLE) {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset()) { error(F("Couldn't factory reset")); }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Change device name (as seen by other devices) */
  //ble.sendCommandCheckOK(F("AT+GAPDevName=WPB Mouse"));

  /* Reset to apply changes */
  //if(!ble.reset()){ error(F("Could not reset??")); }

  /* Enable HID Service (including Mouse) */
  if(!ble.sendCommandCheckOK(F("AT+BleHIDEn=On"))){ error(F("Failed to enable HID (firmware >= 0.6.6?)")); }
}
int c = 0; /**DELETE**************************************************/
// Loop ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  /* Assign x,y values to dX and dY */
  if (Sensor.readReg(Sensor.MOTION_ST) == 0x80) { Sensor.readDeltaXYHigh(); }
  else { Sensor.dX = Sensor.dY = 0; }

  /* Clear motion bit */
  Sensor.writeReg(Sensor.MOTION_ST, 0x00);

  
  /* Construct AT command string to move mouse cursor*/
  String cmd = "AT+BleHIDMouseMove=" + String(toSignedInt(Sensor.dX)) + "," + String(-toSignedInt(Sensor.dY));
  
  /* Append mouse wheel scroll amount to the constructed AT command*/
  if(counter != 0){
    cmd += "," + String(counter);
    counter = 0;
  }
  
  /* Move mouse cursor */
  ble.sendCommandCheckOK(cmd);
  
  
  /* Check and modify state of mouse buttons */
  buttonState(L_BUTTON, LEFT,   &leftIsPressed);
  buttonState(R_BUTTON, RIGHT,  &rightIsPressed);
  buttonState(M_BUTTON, MIDDLE, &middleIsPressed);
}

// User Functions //////////////////////////////////////////////////////////////////////////////////////////////////////////
void error(const __FlashStringHelper *err) {
  Serial.println(err);
  while (1);
}

int toSignedInt(int val){ return (val > 127) ? (val - 256) : val; }

void buttonState(int buttonPin, mouseButton button, bool *buttonIsPressed) {
  if (digitalRead(buttonPin) == HIGH && !(*buttonIsPressed)) {
    *buttonIsPressed = true;
    switch(button) {
      case LEFT:   ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=L")); break;
      case RIGHT:  ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=R")); break;
      case MIDDLE: ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=M")); break;
    }
  }
  else if (digitalRead(buttonPin) == LOW && *buttonIsPressed) {
    *buttonIsPressed = false;
    ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=0"));
  }
}

void mouseWheelTurn(){
  currentState = (digitalRead(MOUSE_WHEEL_A) << 1) + digitalRead(MOUSE_WHEEL_B);  
  if(currentState == lastState){ return; }
  switch(currentState){
    case 0: counter += (lastState == 1) ? SCROLL_FACTOR : ((lastState == 2) ? -SCROLL_FACTOR : 0); break;
    case 1: counter += (lastState == 3) ? SCROLL_FACTOR : ((lastState == 0) ? -SCROLL_FACTOR : 0); break;
    case 3: counter += (lastState == 2) ? SCROLL_FACTOR : ((lastState == 1) ? -SCROLL_FACTOR : 0); break;
    case 2: counter += (lastState == 0) ? SCROLL_FACTOR : ((lastState == 3) ? -SCROLL_FACTOR : 0); break;
  }
  lastState = currentState;
}


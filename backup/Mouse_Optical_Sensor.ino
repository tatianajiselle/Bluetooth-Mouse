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
#define WHEEL                     9
#define L_BUTTON                  10
#define R_BUTTON                  11
#define M_BUTTON                  12
#define FACTORYRESET_ENABLE       0
#define MINIMUM_FIRMWARE_VERSION  "0.6.6"

// Declarations ////////////////////////////////////////////////////////////////////////////////////////////////////////////
A3000 Sensor;
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

bool leftIsPressed = false;
bool rightIsPressed = false;
bool middleIsPressed = false;
enum mouseButton {LEFT, RIGHT, MIDDLE};

// Setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(L_BUTTON, INPUT);
  pinMode(R_BUTTON, INPUT);
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

  /* Change device name (as seen by there devices) */
  //ble.sendCommandCheckOK(F("AT+GAPDevName=WPB Mouse"));

  /* Reset to apply changes */
  //if(!ble.reset()){ error(F("Could not reset??")); }

  /* Enable HID Service (including Mouse) */
  if(!ble.sendCommandCheckOK(F("AT+BleHIDEn=On"))){ error(F("Failed to enable HID (firmware >= 0.6.6?)")); }
}

// Loop ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  char c[40];
  String s;

  /* Assign x,y values to dX and dY */
  if (Sensor.readReg(Sensor.MOTION_ST) == 0x80) { Sensor.readDeltaXYHigh(); }
  else { Sensor.dX = Sensor.dY = 0; }

  /* Clear motion bit */
  Sensor.writeReg(Sensor.MOTION_ST, 0x00);

  /* Cosntruct AT command string to move mouse cursor */
  s = String("AT+BleHIDMouseMove=" + String(toSignedInt(Sensor.dX)) + "," + String(-toSignedInt(Sensor.dY)));
  s.toCharArray(c, sizeof(c));

  /* Move mouse cursor */
  ble.sendCommandCheckOK(c);

  /* Check and modify state of mouse buttons */
  buttonState(L_BUTTON, LEFT,  &leftIsPressed);
  buttonState(R_BUTTON, RIGHT, &rightIsPressed);
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
    ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=0"));
    *buttonIsPressed = false;
  }
}


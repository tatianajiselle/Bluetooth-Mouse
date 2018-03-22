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
#define HOLD_TIME                 10000

// Global Variables ////////////////////////////////////////////////////////////////////////////////////////////////////////
A3000 Sensor;
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

String cmd;
volatile byte lastState = 0;
volatile byte currentState = 0;
volatile int scrollCounter = 0;
bool leftIsPressed = false;
bool rightIsPressed = false;
bool middleIsPressed = false;
bool factoryResetInProgress = false;
unsigned long resetTime = 0;

// Setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
	pinMode(L_BUTTON, INPUT);
	pinMode(R_BUTTON, INPUT);
	pinMode(M_BUTTON, INPUT);
	attachInterrupt(digitalPinToInterrupt(MOUSE_WHEEL_A), scrollMouseWheel, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOUSE_WHEEL_B), scrollMouseWheel, CHANGE);
	Sensor.init();
	Serial.begin(115200);
	if (!ble.begin(VERBOSE_MODE)) { error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?")); }
}

// Loop ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
	/* Assign x,y values to dX and dY */
	if (Sensor.readReg(Sensor.MOTION_ST) == 0x80) { Sensor.readDeltaXYHigh(); }
	else { Sensor.dX = Sensor.dY = 0; }

	/* Clear motion bit */
	Sensor.writeReg(Sensor.MOTION_ST, 0x00);

	/* Construct AT command string to move mouse cursor */
	cmd = "AT+BleHIDMouseMove=" + String(toSignedInt(Sensor.dX)) + "," + String(-toSignedInt(Sensor.dY));

	/* Append scroll amount to constructed AT command, send command (move mouse and / or scroll)... */
	if (scrollCounter != 0) {
		ble.sendCommandCheckOK(cmd += "," + String(scrollCounter));
		scrollCounter = 0;
	}
	/* ... or, just send command (move mouse cursor only) */
	else { ble.sendCommandCheckOK(cmd); }
  
	/* Check and modify state of mouse buttons */
	buttonState(L_BUTTON, "L", &leftIsPressed);
	buttonState(R_BUTTON, "R", &rightIsPressed);
	buttonState(M_BUTTON, "M", &middleIsPressed);
  
	/* Perform a Factory Reset and restore default settings */
	if (leftIsPressed && rightIsPressed && middleIsPressed) {
		if (resetTime == 0) { resetTime = millis(); }
		else if (millis() - resetTime >= HOLD_TIME) {
			resetSettings();
			resetTime = 0;
		}
	}
	else { resetTime = 0; }
}

// User Functions //////////////////////////////////////////////////////////////////////////////////////////////////////////
void error(const __FlashStringHelper *err) {
	Serial.println(err);
	while (1);
}

int toSignedInt(int val) { return (val > 127) ? (val - 256) : val; }

void buttonState(int buttonPin, String button, bool *buttonIsPressed) {
	if (digitalRead(buttonPin) == HIGH && !(*buttonIsPressed)) {
		*buttonIsPressed = true;
		ble.sendCommandCheckOK(cmd = "AT+BleHIDMouseButton=" + button);
	}
	else if (digitalRead(buttonPin) == LOW && *buttonIsPressed) {
		*buttonIsPressed = false;
		ble.sendCommandCheckOK(F("AT+BleHIDMouseButton=0"));
	}
}

void scrollMouseWheel() {
	currentState = (digitalRead(MOUSE_WHEEL_A) << 1) + digitalRead(MOUSE_WHEEL_B);
	if (currentState == lastState) { return; }
	switch (currentState) {
		case 0: scrollCounter += (lastState == 1) ? SCROLL_FACTOR : ((lastState == 2) ? -SCROLL_FACTOR : 0); break;
		case 1: scrollCounter += (lastState == 3) ? SCROLL_FACTOR : ((lastState == 0) ? -SCROLL_FACTOR : 0); break;
		case 3: scrollCounter += (lastState == 2) ? SCROLL_FACTOR : ((lastState == 1) ? -SCROLL_FACTOR : 0); break;
		case 2: scrollCounter += (lastState == 0) ? SCROLL_FACTOR : ((lastState == 3) ? -SCROLL_FACTOR : 0); break;
	}
	lastState = currentState;
}

void resetSettings() {
	/* Perform a factory reset to make sure everything is in a known state */
	Serial.println(F("Performing a factory reset: "));
	if (!ble.factoryReset()) { error(F("Couldn't factory reset")); }
	delay(1000);

	/* Change device name (as seen by other devices) */
	ble.sendCommandCheckOK(F("AT+GAPDevName=WPB Mouse"));

	/* Enable HID Service (including Mouse) */
	if (!ble.sendCommandCheckOK(F("AT+BleHIDEn=On"))) { error(F("Failed to enable HID (firmware >= 0.6.6?)")); }

	/* Disable command echo from Bluefruit */
	ble.echo(false);

	/* Reset to apply changes */
	if (!ble.reset()) { error(F("Could not reset??")); }
}

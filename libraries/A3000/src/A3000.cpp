#include <A3000.h>
#include <SPI.h>

// Defines /////////////////////////////////////////////////////////////////////////////////////////
#define WRITE_BYTE 	0x80 // When added to the address, sets MSB to 1 to indicate a write sequence
#define RESET_VALUE	0x5A // Write this into RESET register for a chip reset
#define DUMMY_BYTE	0x00

// Contructors /////////////////////////////////////////////////////////////////////////////////////

A3000::A3000(void){
	dX = dY = 0;
	setClockFreq(1000000); // Max value - 1MHz
	setDataOrder(MSBFIRST);
	setDataMode(SPI_MODE3);
}

A3000::A3000(long freq, int order, int mode){
	dX = dY = 0;
	setClockFreq(freq);		// Value in Hz
	setDataOrder(order); 	// MSBFIRST, LSBFIRST
	setDataMode(mode);		// SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
}

// Public Methods //////////////////////////////////////////////////////////////////////////////////
void A3000::init(void){
	pinMode(SS, OUTPUT);
	digitalWrite(SS, HIGH);
	SPI.begin();
}

void A3000::setClockFreq(long freq){ _speedMaximum = freq; }

void A3000::setDataOrder(int order){ _dataOrder = order; }

void A3000::setDataMode(int mode){ _dataMode = mode; }

void A3000::writeReg(regAddr reg, byte value){
	SPI.beginTransaction(SPISettings(getClockFreq(), getDataOrder(), getDataMode()));
	digitalWrite(SS, LOW);
	delayMicroseconds(1);
  	SPI.transfer16(((WRITE_BYTE + reg) << 8) + value);
  	delayMicroseconds(1);
  	digitalWrite(SS, HIGH);
  	SPI.endTransaction();
}

byte A3000::readReg(regAddr reg){
	byte temp;
	SPI.beginTransaction(SPISettings(getClockFreq(), getDataOrder(), getDataMode()));
	digitalWrite(SS, LOW);
	delayMicroseconds(1);
	SPI.transfer(reg);
	delayMicroseconds(1);
	temp = SPI.transfer(DUMMY_BYTE);
	delayMicroseconds(1);
	digitalWrite(SS, HIGH);
	SPI.endTransaction();
	return temp;
}

void A3000::readDeltaXY(void){
	dX = readReg(DELTA_X);
	dY = readReg(DELTA_Y);
}

void A3000::readDeltaXYHigh(void){
	byte x, y, h;
	x = readReg(DELTA_X);
	y = readReg(DELTA_Y);
	h = readReg(DELTA_XY_HIGH);
	dX = ((h >> 4) << 8) + x;
	dY = ((h << 12) >> 4) + y;
}

void A3000::reset(void){ 
	writeReg(RESET, RESET_VALUE);
	delay(60);
}

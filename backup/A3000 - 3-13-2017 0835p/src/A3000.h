#ifndef A3000_h
#define A3000_h

#include <Arduino.h>

class A3000{
public:
	int dX;
	int dY;
	
	// Register Addresses //////////////////////////////////////////////////////
	enum regAddr : byte{
		PROD_ID				= 0x00, // Product ID
		REV_ID				= 0x01, // Revision ID
		MOTION_ST			= 0x02, // Motion Status
		DELTA_X				= 0x03, // Lower Byte of Delta_X
		DELTA_Y				= 0x04, // Lower Byte of Delta_Y
		SQUAL				= 0x05, // Squal (Surface Quality)
		SHUT_HI				= 0x06, // Shutter Open Time (Upper 8-bit)
		SHUT_LO				= 0x07, // Shutter Open Time (lower 8-bit)
		PIX_MAX				= 0x08, // Maximum Pixel Value
		PIX_ACCUM			= 0x09, // Average Pixel Value
		PIX_MIN				= 0x0A, // Minimum Pixel Value
		PIX_GRAB			= 0x0B, // Pixel Grabber
		DELTA_XY_HIGH		= 0x0C, // Upper 4 bits of Delta X and Y displacment
		MOUSE_CTRL			= 0x0D, // Mouse Control
		RUN_DOWNSHIFT		= 0x0E, // Run to Rest1 time
		REST1_PERIOD		= 0x0F, // Rest1 Period
		REST1_DOWNSHIFT		= 0x10, // Rest1 to Rest2 Time
		REST2_PERIOD		= 0x11, // Rest2 Period
		REST2_DOWNSHIFT		= 0x12, // Rest2 to Rest3 Time
		REST3_PERIOD		= 0x13, // Rest3 Period
		PERFORMANCE			= 0x22, // Performance
		RESET				= 0x3A, // Reset
		NOT_REV_ID			= 0x3F, // Inverted Revision ID
		LED_CTRL			= 0x40, // LED Control
		MOTION_CTRL			= 0x41, // Motion Control
		BURST_READ_FIRST 	= 0x42, // Burst Read Starting Register
		REST_MODE_CONFIG	= 0x45  // Rest Mode Configuration
	};

	A3000(void);
	A3000(long freq, int order, int mode);

	void init(void);

	long getClockFreq(void){ return _speedMaximum; }
	void setClockFreq(long freq);
	
	int getDataOrder(void){ return _dataOrder; }
	void setDataOrder(int order);
	
	int getDataMode(void){ return _dataMode; }
	void setDataMode(int mode);

	void writeReg(regAddr reg, byte value);
	byte readReg(regAddr reg);

	void readDeltaXY(void);
	void readDeltaXYHigh(void);
	void reset(void);
private:
	long _speedMaximum;
	int  _dataOrder;
	int  _dataMode;
};
#endif

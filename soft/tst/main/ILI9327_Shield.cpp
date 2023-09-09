/* FILE:    ILI9327_SHIELD.cpp
   DATE:    05/04/19
   VERSION: 0.1
   AUTHOR:  Andrew Davies
   
05/04/19 version 0.1: Original version

This library adds hardware support to the HCDisplay library for ILI9341 based screens using the controllers SPI interface.
Current supported boards:

Open-Smart 3.2 Inch TFT shield with touch screen (HCARDU0111)

This library is provided free to support the open source community. PLEASE SUPPORT HOBBY COMPONENTS 
so that we can continue to provide free content like this by purchasing items from our store - 
HOBBYCOMPONENTS.COM 


You may copy, alter and reuse this code in any way you like, but please leave
reference to HobbyComponents.com in your comments if you redistribute this code.
This software may not be used directly for the purpose of selling products that
directly compete with Hobby Components Ltd's own range of products.
THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
REASON WHATSOEVER.
*/



#include "ILI9327_SHIELD.h"

int _FlipMode;
int _res_x;
int	_res_y;

#define _RS_COMMAND  gpio_set_level(PIN_NUM_LCD_RS, 0)
#define _RS_DATA gpio_set_level(PIN_NUM_LCD_RS, 1)

#define _CS_EN  gpio_set_level(PIN_NUM_LCD_CS, 0)
#define _CS_DIS gpio_set_level(PIN_NUM_LCD_CS, 1)

#define delay(v) vTaskDelay((v)/portTICK_PERIOD_MS ? (v)/portTICK_PERIOD_MS : 1))

void _WRITE_DATA(uint8_t d);


/* Resets the interface */
void DReset(void)
{
	_CS_EN;
	
	_WriteCommand(REG_RESET); //Soft reset

	_CS_DIS; 
}

// Writes the initial setting to the screen. Settings are taken from the ILI9325 application note.
void init_screen(void)
{
	DReset();	

	_CS_EN;
	
	_WriteCommand(REG_DISPOFF);
	_WriteCommand(REG_EXITSLEEP);
	delay(120); //Must wait 120ms after exiting sleep 
	_WriteCommand(REG_CMDPROTECT);
	_WriteData(0x00);
	_WriteCommand(REG_DISPTIMING);
	_WriteData(0x10);
	_WriteData(0x10);
	_WriteData(0x02);
	_WriteData(0x02);
	_WriteCommand(REG_PANELDRIVE);
	_WriteData(0x00);
	_WriteData(0x35);
	_WriteData(0x00);
	_WriteData(0x00);
	_WriteData(0x01);
	_WriteData(0x02);
	_WriteCommand(REG_FRAMERATE);
	_WriteData(0x04);
	_WriteCommand(REG_PWRNORMMODE);
	_WriteData(0x01);
	_WriteData(0x04);
	_WriteCommand(REG_3GAMMA);
	_WriteData(0x80);
//	_WriteCommand(REG_ADDRMODE);
//	_WriteData(0x48);
	_WriteCommand(REG_PIXELFORMAT);
	_WriteData(0x55);
	_WriteCommand(REG_COLADDR);
	_WriteData(0x00);
	_WriteData(0x00);
	_WriteData(0x00);
	_WriteData(0xEF);
	_WriteCommand(REG_PAGEADDR);
	_WriteData(0x00);
	_WriteData(0x00);
	_WriteData(0x01);
	_WriteData(0x8F);
	_WriteCommand(REG_PARAREA);
	_WriteData(0x00);
	_WriteData(0x00);
	_WriteData(0x01);
	_WriteData(0x8F);
	_WriteCommand(REG_DISPON);
	
	_CS_DIS;
}






/* Places the screen in to or out of sleep mode where:
	mode is the required state. Valid values area
		true (screen will enter sleep state)
		false (screen will wake from sleep state)
*/
void DSleep(boolean mode)
{
	_CS_EN;
	
	if(mode)
	{
		_WriteCommand(REG_ENTERSLEEP);
		delay(5);
	}
	else
	{
		_WriteCommand(REG_EXITSLEEP);
		delay(120);
	}
	
	_CS_DIS;
} 
	



/* Turn the screen on or off where:
	mode is the required state. Valid values are
		ON (screen on)
		OFF (screen off)
*/
void DScreen(boolean mode)
{
	_CS_EN;
	
	if(mode)
		_WriteCommand(REG_DISPON);	//Screen on
	else
		_WriteCommand(REG_DISPOFF);  //Screen off
	
	_CS_DIS;
}




/* Sets the screen orientation and write direction where:
	mode is the direction, orientation to set the screen to. Valid vales are:
		SCREEN_NORMAL 		(Default)
		SCREEN_R90 			Screen is rotated 90o		
		SCREEN_R180 		Screen is rotated 180o
		SCREEN_R270 		Screen is rotated 270o		
*/
void DFlip(uint8_t mode)
{
	switch(mode)
	{
		case(SCREEN_NORMAL):
			_CS_EN;
			_FlipMode = mode;
			_res_x = RES_X;
			_res_y = RES_Y;
			LCD_CtrlWrite_ILI9327(REG_ADDRMODE, 0b00001000); 
			_CS_DIS;
			break;
			
		case(SCREEN_R90):
			_CS_EN;
			_FlipMode = mode;
			_res_x = RES_Y;
			_res_y = RES_X;
			LCD_CtrlWrite_ILI9327(REG_ADDRMODE, 0b10101000); 
			_CS_DIS;
			break;
			
		case(SCREEN_R180):
			_CS_EN;
			_FlipMode = mode;
			_res_x = RES_X;
			_res_y = RES_Y;
			LCD_CtrlWrite_ILI9327(REG_ADDRMODE, 0b11001000); 
			_CS_DIS;
			break;
			
		case(SCREEN_R270):
			_CS_EN;
			_FlipMode = mode;
			_res_x = RES_Y;
			_res_y = RES_X;
			LCD_CtrlWrite_ILI9327(REG_ADDRMODE, 0b01101000); 
			_CS_DIS;
			break;	
	}
}




/* Draws a solid rectangle to the using the current foreground colour where:
	x1 is the x coordinate of the top left corner of the rectangle.
	y1 is the y coordinate of the top left corner of the rectangle.
	x2 is the x coordinate of the bottom right corner of the rectangle.
	y2 is the y coordinate of the bottom right corner of the rectangle.
*/
void DRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t col1, uint8_t col2)
{
	if(x1 < _res_x && x2 >= 0 && y1 < _res_y && y2 >= 0) 
	{
		_CS_EN;
		
		if(x1 < 0)
			x1 = 0;
		if(y1 < 0)
			y1 = 0;
		if(x2 >= _res_x)
			x2 = _res_x - 1;
		if(y2 >= _res_y)
			y2 = _res_y - 1;
	
		_SetWriteArea(x1, y1, x2, y2);

		_WriteCommand(REG_WRITEMEM); //Write to RAM
	
		for(uint16_t i = x1; i <= x2; i++)
			for(uint16_t i = y1; i <= y2; i++)
			{
				_WriteData(col2); 
				_WriteData(col1); 
			}

		_CS_DIS;
	}
}

void _SetWriteArea(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2)
{
	//Make sure none of the area to write to is outside the display area. If so then crop it
	if(X1 >= _res_x)
		X1 = _res_x - 1;
	if(X2 >= _res_x)
		X2 = _res_x - 1;
	if(Y1 >= _res_y)
		Y1 = _res_y - 1;
	if(Y2 >= _res_y)
		Y2 = _res_y - 1; 

	uint16_t temp;
	
	//Adjust screen offset depending or orientation
	switch(_FlipMode)
	{
		case(SCREEN_NORMAL):
		case(SCREEN_R270):
			break;
			
		case(SCREEN_R90):
			Y1 = Y1 + 32;
			Y2 = Y2 + 32;
			break;
			
		case(SCREEN_R180):
			X1 = X1 + 32;
			X2 = X2 + 32;
			break;
	}
	
	//Set the memory write window
	_WriteCommand(REG_COLADDR);
	_WriteData(Y1>>8);
	_WriteData(Y1);
	_WriteData(Y2>>8);
	_WriteData(Y2);
	_WriteCommand(REG_PAGEADDR);
	_WriteData(X1>>8);
	_WriteData(X1);
	_WriteData(X2>>8);
	_WriteData(X2);
	  
}



/* Internal function that writes 16 bits of data to one of the displays registers */
void LCD_CtrlWrite_ILI9327(uint8_t command, uint8_t data)
{
	_WriteCommand(command);
	_WriteData(data);
}



/* Internal function writes an 8 bit command byte to the display */
void _WriteCommand(uint8_t command)
{
	_RS_COMMAND;
	_WRITE_DATA(command);
}



/* Internal function writes an 8 bit data parameter byte to the display */
void _WriteData(uint8_t data)
{
	_RS_DATA;

	_WRITE_DATA(data);
}


void init(void)
{
	
	/* Configure the data and control pins */
	_DATA_DIR_OUTPUT;
  
	RD_DDR |= RD_MASK;
	WR_DDR |= WR_MASK;
	RS_DDR |= RS_MASK;
	CS_DDR |= CS_MASK;
	
	_RD_HIGH;
	_WR_HIGH;
	_RS_DATA;
	_CS_DIS;
	
	delay(50);
	
	// Write initial setting to the screen
	init_screen();
	
	// Set screen orientation to normal position 
	DFlip(SCREEN_NORMAL);
	
	//Clear the screen
    DRect(0, 0, _res_x, _res_y, 0, 0);


}

# if 0



/* Constructor to initiliase the optional touch library for the TSC2046 touch controller*/
Touch::Touch(void)
{
}



/* Initialises the touch screen controller and configures the control pins where:
	
	DIN is the digital pin used to connect to the controllers data in pin.
	DOUT is the digital pin used to connect to the controllers data out pin.
	CLK is the digital pin used to connect to the controllers clock pin.
	CS is the digital pin used to connect to the controllers chip select pin.
	
	Cal_X_Min is the raw minimum value from the touch screens x axis.
	Cal_X_Max is the raw maximum value from the touch screens x axis.
	Cal_Y_Min is the raw minimum value from the touch screens y axis.
	Cal_Y_Max is the raw maximum value from the touch screens y axis.
	
	Mapping is used to tell the library how the x & y axis of the touch senor has been mapped to the controller. Valid values for Mapping are:
		TOUCH_0_NORMAL 		Normal position
		TOUCH_90_NORMAL 	Sensor is rotated 90o
		TOUCH_180_NORMAL 	Sensor is rotated 180o
		TOUCH_270_NORMAL 	Sensor is rotated 270o
		TOUCH_0_FLIP_X 		X axis is in reverse direction
		TOUCH_90_FLIP_X 	X axis is in reverse direction - sensor is rotated 90o
		TOUCH_180_FLIP_X 	X axis is in reverse direction - sensor is rotated 180o
		TOUCH_270_FLIP_X 	X axis is in reverse direction - sensor is rotated 270o

		TOUCH_0_FLIP_Y 		Y axis is in reverse direction (same as TOUCH_180_FLIP_X)
		TOUCH_90_FLIP_Y 	Y axis is in reverse direction - sensor is rotated 90o (same as TOUCH_270_FLIP_X)
		TOUCH_180_FLIP_Y 	Y axis is in reverse direction - sensor is rotated 180o (same as TOUCH_0_FLIP_X)
		TOUCH_270_FLIP_Y 	Y axis is in reverse direction - sensor is rotated 270o (same as TOUCH_90_FLIP_X)
		TOUCH_0_FLIP_XY 	X & Y axis is in reverse direction (same as TOUCH_180_NORMAL)
		TOUCH_90_FLIP_XY 	X & Y axis is in reverse direction - sensor is rotated 90o (same as TOUCH_270_NORMAL)
		TOUCH_180_FLIP_XY 	X & Y axis is in reverse direction - sensor is rotated 180o (same as TOUCH_0_NORMAL)
		TOUCH_270_FLIP_XY 	X & Y axis is in reverse direction - sensor is rotated 270o (same as TOUCH_90_NORMAL)	
		
	Note that first 8 options for Mapping cover all possible orientations of the sensor. If you are not sure how your sensor is mapped
	it is recommended to try each of the first 8 options in succession until the correct mapping is found. The sensor will be mapped correctly 
	when the raw X axis coordinate reported from the sensor increases from left to right and the raw Y axis coordinate increases from top to bottom.
*/
//void Touch::TInit(uint8_t DIN, uint8_t DOUT, uint8_t CLK, uint8_t CS, uint16_t Cal_X_Min, uint16_t Cal_X_Max, uint16_t Cal_Y_Min, uint16_t Cal_Y_Max, uint8_t Mapping)
void Touch::TInit(uint16_t Cal_X_Min, uint16_t Cal_X_Max, uint16_t Cal_Y_Min, uint16_t Cal_Y_Max, uint8_t Mapping)
{
	//Save the calibration data (raw min/max x/y axis data) and sensor mapping. 
	_Cal_X_Min = Cal_X_Min;
	_Cal_X_Max = Cal_X_Max;
	_Cal_Y_Min = Cal_Y_Min;
	_Cal_Y_Max = Cal_Y_Max;
	
	_Touch_Mapping = Mapping;
}



/* Perform an X & Y axis read of the touch sensor. The results can then be obtained using the TReadAxis() function */
void Touch::TReadTouch(void)
{
	/* If touch sensor is rotated by 90 or 270 then flip the x & y axis, otherwise recored the sensor readings */
	if(_Touch_Mapping == TOUCH_0_NORMAL ||
	   _Touch_Mapping == TOUCH_180_NORMAL ||
	   _Touch_Mapping == TOUCH_0_FLIP_X ||
	   _Touch_Mapping == TOUCH_180_FLIP_X)
	{
		_TouchY = TReadRaw(TOUCH_AXIS_X);
		_TouchX = TReadRaw(TOUCH_AXIS_Y);
	}else
	{
		_TouchX = TReadRaw(TOUCH_AXIS_X);
		_TouchY = TReadRaw(TOUCH_AXIS_Y);
	}
	

	//Depending on the orientation of the sensor, map the raw XY coordinates to the resolution of the screen.
	switch(_Touch_Mapping)
	{
		case(TOUCH_0_NORMAL):
		case(TOUCH_270_FLIP_X):
			_TouchX = constrain(map(_TouchX, _Cal_X_Min, _Cal_X_Max, 0, _res_x - 1), 0 , _res_x - 1);
			_TouchY = constrain(map(_TouchY, _Cal_Y_Max, _Cal_Y_Min, 0, _res_y - 1), 0 , _res_y - 1);
			break;
			
		case(TOUCH_90_NORMAL):
		case(TOUCH_180_FLIP_X):
			_TouchX = constrain(map(_TouchX, _Cal_X_Max, _Cal_X_Min, 0, _res_x - 1), 0 , _res_x - 1);
			_TouchY = constrain(map(_TouchY, _Cal_Y_Max, _Cal_Y_Min, 0, _res_y - 1), 0 , _res_y - 1);
			break;
			
		case(TOUCH_180_NORMAL):
		case(TOUCH_90_FLIP_X):
			_TouchX = constrain(map(_TouchX, _Cal_X_Max, _Cal_X_Min, 0, _res_x - 1), 0 , _res_x - 1);
			_TouchY = constrain(map(_TouchY, _Cal_Y_Min, _Cal_Y_Max, 0, _res_y - 1), 0 , _res_y - 1);
			break;
			
		case(TOUCH_270_NORMAL):
		case(TOUCH_0_FLIP_X):
			_TouchX = constrain(map(_TouchX, _Cal_X_Min, _Cal_X_Max, 0, _res_x - 1), 0 , _res_x - 1);
			_TouchY = constrain(map(_TouchY, _Cal_Y_Min, _Cal_Y_Max, 0, _res_y - 1), 0 , _res_y - 1);
			break;	
	}
}




/* Checks to see if the screen is currently pressed.

	Returns a boolean value where:
	
	true = screen is currently pressed.
	false = screen is not currently pressed.
*/
boolean Touch::TPressed(void)
{
	pinMode(17, OUTPUT);
	digitalWrite(17, HIGH);

	boolean state = true;
	TOUCH_XP_DDR |= TOUCH_XP_MASK;
	TOUCH_XN_DDR &= ~TOUCH_XN_MASK;
	TOUCH_YP_DDR &= ~TOUCH_YP_MASK;
	TOUCH_YN_DDR &= ~TOUCH_YN_MASK;
	
	TOUCH_XP_LOW;

		
	if(analogRead(TOUCH_Y_ANALOGUE) > /*(min(_Cal_X_Min, _Cal_X_Max))*/ 512)
		state = false;
	TOUCH_XP_HIGH;
	
	if(analogRead(TOUCH_Y_ANALOGUE) < /*(min(_Cal_X_Min, _Cal_X_Max))*/ 512)
		state = false;
	TOUCH_XN_DDR |= TOUCH_XN_MASK;
	TOUCH_YP_DDR |= TOUCH_YP_MASK;
	TOUCH_YN_DDR |= TOUCH_YN_MASK;

	return state;
}




/* Returns the last X or Y axis reading from the touch sensor as a value mapped to the resolution of the screen where:

	Axis is a boolean value specifying which axis reading to return. Valid values for Axis are:

		TOUCH_AXIS_X - Specifies the X axis
		TOUCH_AXIS_Y - Specifies the Y axis	
*/
uint16_t Touch::TGetCoord(boolean Axis)
{
	if(Axis == TOUCH_AXIS_X)
		return _TouchX;
	else
		return _TouchY;
}



/* Triggers a sensor measurement of one of the axis and stores the result where:

	Axis is a boolean value specifying which axis to measure. Valid values for Axis are:

		TOUCH_AXIS_X - Specifies the X axis
		TOUCH_AXIS_Y - Specifies the Y axis	
		
	Returns an unsigned int value containing a raw sensor reading for the specified axis.
*/
uint16_t Touch::TReadRaw(boolean Axis)
{
	uint16_t Result;
	if(Axis == TOUCH_AXIS_X)
	{
		TOUCH_XP_DDR |= TOUCH_XP_MASK;
		TOUCH_XN_DDR |= TOUCH_XN_MASK;
		TOUCH_YP_DDR &= ~TOUCH_YP_MASK;
		TOUCH_YN_DDR &= ~TOUCH_YN_MASK;
	
		TOUCH_XP_HIGH;
		TOUCH_XN_LOW;
		
		Result = analogRead(TOUCH_Y_ANALOGUE);
		
		TOUCH_YP_DDR |= TOUCH_YP_MASK;
		TOUCH_YN_DDR |= TOUCH_YN_MASK;	
	}else
	{
		TOUCH_YP_DDR |= TOUCH_YP_MASK;
		TOUCH_YN_DDR |= TOUCH_YN_MASK;
		TOUCH_XP_DDR &= ~TOUCH_XP_MASK;
		TOUCH_XN_DDR &= ~TOUCH_XN_MASK;
	
		TOUCH_YP_HIGH;
		TOUCH_YN_LOW;
		
		Result = analogRead(TOUCH_X_ANALOGUE);
		
		TOUCH_XP_DDR |= TOUCH_XP_MASK;
		TOUCH_XN_DDR |= TOUCH_XN_MASK;
	}
	
	return Result;
}



/* Internal library function used by the HCDisplay class to pass the screens X & Y resolution to the touch class.
*/
void Touch::SetScreenRes(uint16_t Res_X, uint16_t Res_Y)
{
	_res_x = Res_X;
	_res_y = Res_Y;
}

#endif

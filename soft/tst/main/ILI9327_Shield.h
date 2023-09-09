/* FILE:    ILI9327_SHIELD.h
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

#ifndef ILI9327_SHIELD_h
#define ILI9327_SHIELD_h

#define RES_X 400	//Screen X axis resolution
#define RES_Y 240	//Screen Y axis resolution


/* ILI9327 command registers used in this library */
#define REG_RESET 		0x01
#define REG_ENTERSLEEP 	0x10
#define REG_EXITSLEEP 	0x11
#define REG_DISPOFF 	0x28
#define REG_DISPON 		0x29
#define REG_COLADDR		0x2A
#define REG_PAGEADDR	0x2B
#define REG_WRITEMEM	0x2C
#define REG_READMEM		0x2E
#define REG_PARAREA		0x30
#define REG_ADDRMODE	0x36
#define REG_PIXELFORMAT	0x3A
#define REG_CMDPROTECT 	0xB0
#define REG_PANELDRIVE	0xC0
#define REG_DISPTIMING	0xC1
#define REG_FRAMERATE	0xC5
#define REG_PWRNORMMODE	0xD2
#define REG_3GAMMA		0xEA


#define TOUCH_XP_PORT PORTD
#define TOUCH_YP_PORT PORTC
#define TOUCH_XN_PORT PORTC
#define TOUCH_YN_PORT PORTD

#define TOUCH_XP_DDR DDRD
#define TOUCH_YP_DDR DDRC
#define TOUCH_XN_DDR DDRC
#define TOUCH_YN_DDR DDRD

#define TOUCH_XP_MASK 0b01000000
#define TOUCH_YP_MASK 0b00000010
#define TOUCH_XN_MASK 0b00000100
#define TOUCH_YN_MASK 0b10000000

#define TOUCH_XP_LOW 		TOUCH_XP_PORT &= ~TOUCH_XP_MASK
#define TOUCH_XP_HIGH 		TOUCH_XP_PORT |= TOUCH_XP_MASK

#define TOUCH_YP_LOW  		TOUCH_YP_PORT &= ~TOUCH_YP_MASK
#define TOUCH_YP_HIGH  		TOUCH_YP_PORT |= TOUCH_YP_MASK

#define TOUCH_XN_LOW  		TOUCH_XN_PORT &= ~TOUCH_XN_MASK
#define TOUCH_XN_HIGH  		TOUCH_XN_PORT |= TOUCH_XN_MASK

#define TOUCH_YN_LOW  		TOUCH_YN_PORT &= ~TOUCH_YN_MASK
#define TOUCH_YN_HIGH  		TOUCH_YN_PORT |= TOUCH_YN_MASK

#define TOUCH_Y_ANALOGUE	A1
#define TOUCH_X_ANALOGUE	A2

#endif
#pragma once

#define PIN_NUM_CLK     GPIO_NUM_22
#define PIN_NUM_MOSI    GPIO_NUM_23
#define PIN_NUM_CS_SOL  GPIO_NUM_33
#define PIN_NUM_CS_LCD  GPIO_NUM_25

#define PIN_NUM_OE_SOL  GPIO_NUM_32

#define PIN_NUM_LCD_CS  GPIO_NUM_18
#define PIN_NUM_LCD_BL   GPIO_NUM_5
#define PIN_NUM_LCD_SW1 GPIO_NUM_27
#define PIN_NUM_LCD_SW2 GPIO_NUM_26
#define PIN_NUM_LCD_WR  GPIO_NUM_21
#define PIN_NUM_LCD_RS  GPIO_NUM_19

/*
    CS      - 18
    BL      - 5           LCD    TouchX  TouchY  TouchSence     TouchSence/Alt    Idle
    DB7/Y- (SW1) - 27    1/DB7     0      1/0       1/0             0             1/1
    DB6/X+ (SW2) - 26    1/DB6    1/1      0         0             1/0            1/1
    WR/Y+   - 21          WR      ADC      1         0           PullUP/Sence      1       -> 36 (SENSOR_VP)
    RS/X-   - 19          RS       0      ADC     PullUP/Sence      0              1       -> 39 (SENSOR_VN)
*/


#define PIN_NUM_FP_TXD  GPIO_NUM_34
#define PIN_NUM_FP_RXD  GPIO_NUM_17
#define PIN_NUM_FP_WAKEUP GPIO_NUM_35

#define PIN_NUM_I2C_SDA     GPIO_NUM_16
#define PIN_NUM_I2C_SCL      GPIO_NUM_4

#define PIN_NUM_DIPSW_BOOT   GPIO_NUM_0
#define PIN_NUM_DIPSW_AUX    GPIO_NUM_2

#define ADC_Y_PLUS  ADC_CHANNEL_0
#define ADC_X_MINUS ADC_CHANNEL_3


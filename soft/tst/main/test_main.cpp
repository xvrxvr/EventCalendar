#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/uart.h"

#include "ILI9327_SHIELD.h"
#include "pins.h"

#define LCD_HOST    HSPI_HOST

enum {
   	SCREEN_NORMAL,
   	SCREEN_R90,
   	SCREEN_R180,
   	SCREEN_R270
};

spi_device_handle_t lcd_spi, sol_spi;
adc_oneshot_unit_handle_t adc1_handle;

int _FlipMode;
int _res_x;
int	_res_y;

#define _RS_COMMAND  gpio_set_level(PIN_NUM_LCD_RS, 0)
#define _RS_DATA gpio_set_level(PIN_NUM_LCD_RS, 1)
#define _LCD_IDLE _RS_DATA

#define _CS_EN  gpio_set_level(PIN_NUM_LCD_CS, 0)
#define _CS_DIS do {gpio_set_level(PIN_NUM_LCD_CS, 1); _LCD_IDLE;} while(0)

#define delay(v) vTaskDelay((v)/portTICK_PERIOD_MS ? (v)/portTICK_PERIOD_MS : 1)

void LCD_CtrlWrite_ILI9327(uint8_t command, uint8_t data);
void _SetWriteArea(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2);

void write_lcd(uint8_t d)
{
    esp_err_t ret;
    spi_transaction_t t{};

    t.length=8;
    t.tx_data[0]=d;
    t.flags = SPI_TRANS_USE_TXDATA;

    ret=spi_device_polling_transmit(lcd_spi, &t);
    assert(ret==ESP_OK);
}

#include <soc/spi_struct.h>
#include <soc/gpio_struct.h>
#include <hal/spi_ll.h>

void write_lcd_fast(uint8_t data)
{
    spi_dev_t *hw = &SPI2;

    hw->slave.trans_done = 0;
/*
    hw->ctrl.val &= ~SPI_LL_ONE_LINE_CTRL_MASK;
    hw->user.val &= ~SPI_LL_ONE_LINE_USER_MASK;
    hw->user.usr_dummy =  0;                                         
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->user1, usr_dummy_cyclelen, 0);
    hw->ctrl2.miso_delay_mode = 0;
    hw->ctrl2.miso_delay_num = 0; 
    hw->mosi_dlen.usr_mosi_dbitlen = 7;
    hw->miso_dlen.usr_miso_dbitlen = 7;
    hw->user1.usr_addr_bitlen = 0;
    hw->user.usr_addr = 0;        
    hw->user2.usr_command_bitlen = 0;
    hw->user.usr_command = 0;        
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->user2, usr_command_value, 0);
    hw->addr = 0;
    hw->pin.cs_keep_active = 0;
*/

    hw->data_buf[0] = data;

//    hw->user.usr_miso = 0;
//    hw->user.usr_mosi = 1;

//    hw->slave.trans_inten = 0;    --- enable/disable Trans done int
    hw->cmd.usr = 1;
    while(!hw->slave.trans_done) {;} // ???

    GPIO.out_w1tc = 1 << PIN_NUM_LCD_WR;
    GPIO.out_w1ts = 1 << PIN_NUM_LCD_WR;
}

void _WRITE_DATA(uint8_t d)
{
    write_lcd(d);
    gpio_set_level(PIN_NUM_LCD_WR, 0);
    gpio_set_level(PIN_NUM_LCD_WR, 1);
}

inline void _WRITE_DATA_FAST(uint8_t d)
{
    write_lcd_fast(d);
//    gpio_set_level(PIN_NUM_LCD_WR, 0);
//    gpio_set_level(PIN_NUM_LCD_WR, 1);
}

void write_sol(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t{};

    t.length=8;
    t.tx_data[0]=data;
    t.flags = SPI_TRANS_USE_TXDATA;

    ret=spi_device_polling_transmit(sol_spi, &t);
    assert(ret==ESP_OK);
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
	_WRITE_DATA_FAST(data);
}


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
void DSleep(bool mode)
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
void DScreen(bool mode)
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


void init(void)
{
printf("Init: before delay\n");	
	delay(50);
	
printf("Init: before init_screen\n");
	// Write initial setting to the screen
	init_screen();
	
printf("Init: before DFlip\n");
	// Set screen orientation to normal position 
	DFlip(SCREEN_NORMAL);
printf("Init: before DRect. _res_x=%d, _res_y=%d\n", _res_x, _res_y);	
	//Clear the screen
	DRect(0, 0, _res_x, _res_y, 0, 0);
printf("Init: on exit\n");
}

//    _BGColour = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
//       <rrrrr> <ggg> | <ggg> <bbbbb>

enum {
    Z1 = 0,     // High (first)
    Z2 = 0,

    W1 = 0xFF,
    W2  =0xFF,

    R1 = 0xF8,
    R2 = 0,

    G1 = 0x07,
    G2 = 0xE0,

    B1 = 0,
    B2 = 0x1F
};


void bench_lcd()
{
#define R(c) DRect(0, 0, _res_x, _res_y, c##1, c##2)
    
        R(Z);
        delay(1000);
        R(R);
        delay(1000);
        R(G);
        delay(1000);
        R(B);
        delay(1000);
        R(W);
        delay(1000);
#if 0
        auto tm = xTaskGetTickCount();
        for(int i=0; i<10; ++i)
        {
            R(Z);
            R(R);
            R(G);
            R(B);
            R(W);
        }
        tm = xTaskGetTickCount() - tm;
        printf("LCD FPS: %lu\n", 50'000/(tm*portTICK_PERIOD_MS));
#endif
}


/*
    BL      - 5           LCD    TouchX TouchXInv TouchY  TouchYInv  TouchSence     TouchSence/Alt    Idle
    DB7/Y- (SW1) - 27    1/DB7     0      0         1/0     1/1          1/0             0             1/1
    DB6/X+ (SW2) - 26    1/DB6    1/1    1/0         0       0            0             1/0            1/1
    WR/Y+   - 21          WR      ADC    ADC         1       0            0           PullUP/Sence      1       -> 36 (SENSOR_VP)
    RS/X-   - 19          RS       0      1         ADC     ADC        PullUP/Sence      0              1       -> 39 (SENSOR_VN)
*/

enum TouchType {
    TT_LCD,
    TT_X,
    TT_XInv,
    TT_Y,
    TT_YInv,
    TT_Sence
};

enum TTSetup {
    TTS_SW1     = 0x01,
    TTS_SW2     = 0x02,
    TTS_WR_ADC  = 0x04,
    TTS_WR      = 0x08,
    TTS_RS_ADC  = 0x10,
    TTS_RS      = 0x20,
    TTS_DB6     = 0x40,   // Do not change value!
    TTS_DB7     = 0x80,   // Do not change value!
    TTS_RS_SNS  = 0x100
};

static const int tts_setup [] = {
    TTS_SW1 | TTS_SW2 | TTS_WR | TTS_RS,         // TT_LCD
    TTS_SW2 | TTS_DB6 | TTS_WR_ADC,              // TT_X
    TTS_SW2 | TTS_WR_ADC | TTS_RS,               // TT_XInv
    TTS_SW1 | TTS_WR | TTS_RS_ADC,               // TT_Y
    TTS_SW1 | TTS_DB7 | TTS_RS_ADC,              // TT_YInv
    TTS_SW1 | TTS_RS_SNS                         // TT_Sence
};

#define NOP() __asm volatile("nop")

int touch_config(TouchType tt)
{
    int read_adc = -1;
    int setup = tts_setup[tt];
#define V(name) ((setup & TTS_##name) != 0)
    write_lcd(setup & 0xC0); // DB6 & DB7
    gpio_set_level(PIN_NUM_LCD_SW1, V(SW1));
    gpio_set_level(PIN_NUM_LCD_SW2, V(SW2));

    if (setup & TTS_WR_ADC) {gpio_set_direction(PIN_NUM_LCD_WR, GPIO_MODE_DISABLE); read_adc = ADC_Y_PLUS;}
    else {gpio_set_level(PIN_NUM_LCD_WR, V(WR)); gpio_set_direction(PIN_NUM_LCD_WR, GPIO_MODE_OUTPUT);}

    if (setup & TTS_RS_SNS) {gpio_set_direction(PIN_NUM_LCD_RS, GPIO_MODE_INPUT); gpio_set_pull_mode(PIN_NUM_LCD_RS, GPIO_PULLUP_ONLY); NOP(); NOP(); return gpio_get_level(PIN_NUM_LCD_RS);} else
    if (setup & TTS_RS_ADC) {gpio_set_pull_mode(PIN_NUM_LCD_RS, GPIO_FLOATING); gpio_set_direction(PIN_NUM_LCD_RS, GPIO_MODE_DISABLE); read_adc = ADC_X_MINUS;} 
    else {gpio_set_level(PIN_NUM_LCD_RS, V(RS)); gpio_set_direction(PIN_NUM_LCD_RS, GPIO_MODE_OUTPUT);}
#undef V

    if (read_adc != -1)
    {
        vTaskDelay(1);
        int value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, (adc_channel_t)read_adc, &value));
        return value;
    }

    return 0;
}


#define _B(n) (1ull<<(n))

bool test_touch()
{
    int v = touch_config(TT_Sence);
    for(int i=0; i<100; ++i)
    {
        if (v != touch_config(TT_Sence)) {v = !v; i=0;}
    }
    return !v;
}


extern "C" void app_main(void)
{
    esp_err_t ret;

    gpio_set_level(PIN_NUM_OE_SOL, 1);
    gpio_set_level(PIN_NUM_LCD_CS, 1);
    gpio_set_level(PIN_NUM_LCD_SW1, 1);
    gpio_set_level(PIN_NUM_LCD_SW2, 1);
    gpio_set_level(PIN_NUM_LCD_WR, 1);
    gpio_set_level(PIN_NUM_LCD_BL, 1);

    gpio_config_t gpio_cfg_out = {
        .pin_bit_mask = _B(PIN_NUM_OE_SOL)|_B(PIN_NUM_LCD_CS)|_B(PIN_NUM_LCD_BL)|_B(PIN_NUM_LCD_SW1)|_B(PIN_NUM_LCD_SW2)|_B(PIN_NUM_LCD_WR)|_B(PIN_NUM_LCD_RS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg_out));
    gpio_config_t gpio_cfg_in = {
        .pin_bit_mask = _B(PIN_NUM_FP_WAKEUP)|_B(PIN_NUM_DIPSW_BOOT)|_B(PIN_NUM_DIPSW_AUX),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg_in));

    gpio_set_level(PIN_NUM_OE_SOL, 1);
    gpio_set_level(PIN_NUM_LCD_CS, 1);
    gpio_set_level(PIN_NUM_LCD_SW1, 1);
    gpio_set_level(PIN_NUM_LCD_SW2, 1);
    gpio_set_level(PIN_NUM_LCD_WR, 1);
    gpio_set_level(PIN_NUM_LCD_BL, 1);

    spi_bus_config_t buscfg={
        .mosi_io_num=PIN_NUM_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=10
    };
    spi_device_interface_config_t lcd_devcfg={
        .mode=0,                                //SPI mode 0
        .clock_speed_hz=SPI_MASTER_FREQ_80M,           //Clock out at 26 MHz
        .spics_io_num=PIN_NUM_CS_LCD,               //CS pin
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size=1
    };
    spi_device_interface_config_t sol_devcfg={
        .mode=0,                                //SPI mode 0
        .clock_speed_hz=SPI_MASTER_FREQ_10M,           //Clock out at 26 MHz
        .spics_io_num=PIN_NUM_CS_SOL,               //CS pin
        .queue_size=1
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(LCD_HOST, &lcd_devcfg, &lcd_spi);
    ESP_ERROR_CHECK(ret);

    //Attach the Solenoids to the SPI bus
    ret=spi_bus_add_device(LCD_HOST, &sol_devcfg, &sol_spi);
    ESP_ERROR_CHECK(ret);

    // Zero Solenoid register, then OE it
    write_sol(0);
    gpio_set_level(PIN_NUM_OE_SOL, 0);

    // ADC config
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_Y_PLUS, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_X_MINUS, &config));


    printf("Run init\n");
    touch_config(TT_LCD);

    spi_device_acquire_bus(lcd_spi, portMAX_DELAY);

    //Initialize the LCD
    init();
#if 0
    // LCD test
    printf("LCD Bench started\n");
    bench_lcd();
    spi_device_release_bus(lcd_spi);
#if 0
    // Solenoid test
    printf("Solenoid test\n");
    for(int i=0; i<8; ++i)
    {
        write_sol(1<<i);
        delay(1000);
        write_sol(0);
        delay(1000);
    }
    write_sol(0);
#endif
    // Touch
    printf("Run Touch check. Touch screen to start\n");
    for(int i=0; i<4; ++i)
    {
        while(!test_touch()) {;}
        const char* mm[] = {"X", "~X", "Y", "~Y"};
        for(int touch_type=TT_X; touch_type <= TT_YInv; ++touch_type)
        {
            printf("%s: ", mm[touch_type-TT_X]);
            for(int j=0; j<10;++j)
            {
                printf("%d ", touch_config((TouchType)touch_type));
            }
            printf("\n");
        }
        printf("\n");
        while(test_touch()) {;}
    }
    touch_config(TT_LCD);

    // Configure I2C
    i2c_port_t i2c_master_port = I2C_NUM_0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_NUM_I2C_SDA,
        .scl_io_num = PIN_NUM_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master = {
            .clk_speed = 100'000
        }
    };

    i2c_param_config(i2c_master_port, &conf);

    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0));

#define I2C_MASTER_TIMEOUT   (1000/portTICK_PERIOD_MS)

    {
        // Temperature
        int8_t data[2] = {0};
        ESP_ERROR_CHECK(i2c_master_read_from_device(i2c_master_port, 0x48, (uint8_t *)data, 2, I2C_MASTER_TIMEOUT));
        printf("T is %d\n", data[0]);
    }

    {
        // Clock
        printf("RTC:\n");
        uint8_t ptr=0;
        uint8_t data[8];
        ESP_ERROR_CHECK(i2c_master_write_read_device(i2c_master_port, 0xD0>>1, &ptr, 1, data, 8, I2C_MASTER_TIMEOUT));
        for(int i=0; i<8; ++i)
            printf("%d: %02X\n", i, data[i]);
    }

    {
        // EEPROM
        printf("EEPROM:\n");
        uint8_t data[8];
        ESP_ERROR_CHECK(i2c_master_read_from_device(i2c_master_port, 0xA0>>1, (uint8_t *)&data, 8, I2C_MASTER_TIMEOUT));
        for(int i=0; i<8; ++i)
            printf(" %02X", data[i]);
        printf("\n");
    }

    // BL
    printf("BL\n");
 
    ESP_ERROR_CHECK(ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1));

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PIN_NUM_LCD_BL,
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = (1<<13)-1,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0, 1000, LEDC_FADE_WAIT_DONE));
#endif
    // FP
    {
        printf("FP\n");

        const uart_port_t uart_num = UART_NUM_2;
        uart_config_t uart_config = {
            .baud_rate = 57600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT
        };
        // Configure UART parameters
        ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, PIN_NUM_FP_RXD, PIN_NUM_FP_TXD, -1, -1));

        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 2048, 0, 0, NULL, 0));

	uint8_t data[1024];
        uint8_t sdata[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x13, 0, 0, 0, 0, 0, 0x1B};
        uart_write_bytes(uart_num, (const char*)sdata, sizeof(sdata));

        int length = uart_read_bytes(uart_num, data, 1024, 1000 / portTICK_PERIOD_MS);
        printf("Passwd check, Got:");
        for(int i=0; i<length; ++i)
            printf(" %02X", data[i]);
        printf("\n");

        uint8_t sdata2[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0F, 0, 0x13};
        uart_write_bytes(uart_num, (const char*)sdata2, sizeof(sdata2));

        length = uart_read_bytes(uart_num, data, 1024, 1000 / portTICK_PERIOD_MS);
        printf("Status, Got:");
        for(int i=0; i<length; ++i)
            printf(" %02X", data[i]);
        printf("\n");

        printf("Check WakeUp function\n");
        int prev = -1;
        for(;;)
        {
            int l = gpio_get_level(PIN_NUM_FP_WAKEUP);
            if (l != prev)
            {
                printf("  %d -> %d\n", prev, l);
                prev=l;
            }
        }
    }
}

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>
#include "lcd_drv.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "font.h"
#include "image2lcd.h"
#include "driver/i2c.h"
#include "i2c_task.h"
#include "touch_drv.h"

char debugbuf[100];
u16 POINT_COLOR = 0x0000;
u16 BACK_COLOR  = BLACK;
u8 DFT_SCAN_DIR;

extern const unsigned char gImage_logo;

#define PIN_MOSI   LCD_PIN_MOSI
#define PIN_MISO   LCD_PIN_MISO
#define PIN_SCLK   LCD_PIN_CLK
#define PIN_CS     LCD_PIN_CS   // manual CS
#define PIN_DC     LCD_PIN_DC   // D/C (command = 0 / data = 1)
#define PIN_RST    LCD_PIN_RST  // optional reset pin

// SPI device handle
static spi_device_handle_t spi_dev = NULL;

sLcd_Data_t LcdStruct;
uint8_t CommandBuffer;
uint8_t DataBuffer[20];

// Macros for manual CS control
#define CS_LOW()     gpio_set_level(PIN_CS, 0)
#define CS_HIGH()    gpio_set_level(PIN_CS, 1)
#define DC_COMMAND() gpio_set_level(PIN_DC, 0)
#define DC_DATA()    gpio_set_level(PIN_DC, 1)
#define RST_LOW()    gpio_set_level(PIN_RST, 0)
#define RST_HIGH()   gpio_set_level(PIN_RST, 1)

esp_err_t LCD_Write(const uint8_t *cmd, size_t cmd_len, const uint8_t *data, size_t data_len);

/*
 *@brief Send buffer to LCD via SPI
 * param buf Pointer to data buffer
 * param txlen Length of data buffer in bytes
 * return ESP_OK on success, error code otherwise
 */
esp_err_t Spi_SendBuffer(const uint8_t *buf, size_t txlen)
{
    if (txlen == 0) return ESP_OK;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = txlen * 8;   // bits
    t.tx_buffer = buf;
    t.rx_buffer = NULL;
    return spi_device_transmit(spi_dev, &t);
}

/*
 *@brief Send command to LCD
 * param data Pointer to data buffer
 * param len Length of data buffer in bytes
 * return ESP_OK on success, error code otherwise
 */
void LCD_WR_CMD(const uint8_t *cmd, size_t cmd_len)
{
	esp_err_t ret;

	CS_LOW();
	DC_COMMAND();
	ret = Spi_SendBuffer(cmd, cmd_len);
	CS_HIGH();
	if (ret != ESP_OK) {
		// ESP_LOGE(TAG, "LCD_WR_CMD failed: %s", esp_err_to_name(ret));
		memset(debugbuf,0,sizeof(debugbuf));
		sprintf(debugbuf,"LCD_WR_CMD failed: %s", esp_err_to_name(ret));
		uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
	}
}

/*
 *@brief Send data to LCD
 * param data Pointer to data buffer
 * param len Length of data buffer in bytes
 * return ESP_OK on success, error code otherwise
 */
void LCD_WR_DATA_BUF(const uint8_t *data, size_t data_len)
{
    esp_err_t ret = ESP_OK;
    size_t offset = 0;
    size_t chunk;
    size_t max_cunk_size = LCD_SPI_TX_SIZE - 32;


	CS_LOW();
	DC_DATA();

    while (offset < data_len)
    {
        chunk = data_len - offset;

        if (chunk > max_cunk_size)
            chunk = max_cunk_size;

        ret = Spi_SendBuffer(data + offset, chunk);
        if (ret != ESP_OK)
            break;

        offset += chunk;
    }

	CS_HIGH();
	if (ret != ESP_OK) {
		// ESP_LOGE(TAG, "LCD_WR_DATA_BUF failed: %s", esp_err_to_name(ret));
		memset(debugbuf,0,sizeof(debugbuf));
		sprintf(debugbuf,"LCD_WR_DATA_BUF failed: %s", esp_err_to_name(ret));
		uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
	}
}

/*
 *@brief Send command and data to LCD
 * param cmd Pointer to command buffer
 * param cmd_len Length of command buffer in bytes
 * param data Pointer to data buffer
 * param data_len Length of data buffer in bytes
 * return ESP_OK on success, error code otherwise
 */
esp_err_t LCD_Write(const uint8_t *cmd, size_t cmd_len, const uint8_t *data, size_t data_len)
{
    esp_err_t ret;

    CS_LOW();
    if (cmd_len)
	{
		DC_COMMAND();
        ret = Spi_SendBuffer(cmd, cmd_len);
        if (ret != ESP_OK)
        {
            CS_HIGH(); return ret;
        }
    }

    if (data_len)
    {
        DC_DATA();
        ret = Spi_SendBuffer(data, data_len);
        if (ret != ESP_OK)
        {
            CS_HIGH(); return ret;
        }
    }
    CS_HIGH();
    return ESP_OK;
}

/*
 * @brief Write a command to LCD
 * regval: Command value to write
 */
void LCD_WR_REG(u8 regval)
{
	LCD_WR_CMD(&regval, 1);
}

/*
 * @brief Write data to LCD
 * data: Data value to write
 */
void LCD_WR_DATA(u8 data)
{
	LCD_WR_DATA_BUF(&data, 1);
}

/*
 * @brief Write a 16-bit RGB color to LCD RAM
 * RGB_Code: 16-bit color value to write
 */
void LCD_WriteRAM(u16 RGB_Code)
{
	uint8_t data[2] = {RGB_Code >> 8, RGB_Code & 0xFF};
	LCD_WR_DATA_BUF(data, sizeof(data));
}

/*
 * @brief Prepare to write to LCD RAM
 */
void LCD_WriteRAM_Prepare(void)
{
 	LCD_WR_REG(LcdStruct.wramcmd);
}

/*
 * @brief Set the cursor position on the LCD
 * Xpos: X coordinate
 * Ypos: Y coordinate
 */
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	CommandBuffer = LcdStruct.setxcmd;
  	DataBuffer [0] = Xpos >> 8;
  	DataBuffer [1] = Xpos & 0xFF;
  	DataBuffer [2] = Xpos >> 8;
  	DataBuffer [3] = Xpos & 0xFF;
  	LCD_Write(&CommandBuffer, 1, DataBuffer, 4);

	CommandBuffer = LcdStruct.setycmd;
  	DataBuffer [0] = Ypos >> 8;
  	DataBuffer [1] = Ypos & 0xFF;
  	DataBuffer [2] = Ypos >> 8;
  	DataBuffer [3] = Ypos & 0xFF;
  	LCD_Write(&CommandBuffer, 1, DataBuffer, 4);
}

/*
 * @brief Set the scanning direction of the LCD
 * dir: Scanning direction (0-7)
 */
void LCD_Scan_Dir(u8 dir)
{
    uint8_t madctl = 0;

    switch (dir)
    {
        case LCD_DIR_0:        // 0°
            madctl = MADCTL_MX | MADCTL_BGR;
            break;

        case LCD_DIR_90:       // 90°
            madctl = MADCTL_MV | MADCTL_BGR;
            break;

        case LCD_DIR_180:      // 180°
            madctl = MADCTL_MY | MADCTL_BGR;
            break;

        case LCD_DIR_270:      // 270°
            madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            break;

        default:
            return; // Safety
    }

    /* Write MADCTL */
    CommandBuffer = 0x36;
    LCD_Write(&CommandBuffer, 1, &madctl, 1);

    /* Update column address */
    CommandBuffer = LcdStruct.setxcmd;
    DataBuffer[0] = 0;
    DataBuffer[1] = 0;
    DataBuffer[2] = (LcdStruct.width - 1) >> 8;
    DataBuffer[3] = (LcdStruct.width - 1) & 0xFF;
    LCD_Write(&CommandBuffer, 1, DataBuffer, 4);

    /* Update row address */
    CommandBuffer = LcdStruct.setycmd;
    DataBuffer[0] = 0;
    DataBuffer[1] = 0;
    DataBuffer[2] = (LcdStruct.height - 1) >> 8;
    DataBuffer[3] = (LcdStruct.height - 1) & 0xFF;
    LCD_Write(&CommandBuffer, 1, DataBuffer, 4);
}

/*
 * @brief Draw a point on the LCD at specified coordinates with given color
 * x: X coordinate
 * y: Y coordinate
 * color: Color value
 */
void LCD_DrawPoint(u16 x,u16 y, uint32_t color)
{
	LCD_SetCursor(x,y);		// Set cursor position
	LCD_WriteRAM_Prepare();	// Prepare to write to GRAM
	LCD_WriteRAM(color);
}

/*
 * @brief Set the display direction of the LCD
 * dir: 0 for horizontal, 1 for vertical
 */
void LCD_Display_Dir(u8 dir)
{
    /* Common commands (same for all orientations) */
    LcdStruct.wramcmd = 0x2C;   // RAMWR
    LcdStruct.setxcmd = 0x2A;   // CASET
    LcdStruct.setycmd = 0x2B;   // RASET

    switch (dir)
    {
        case LCD_ORIENT_PORTRAIT:
            LcdStruct.dir    = 0;
            LcdStruct.width  = LCD_WIDTH;
            LcdStruct.height = LCD_HEIGHT;

            /* Portrait scan direction */
            LCD_Scan_Dir(LCD_DIR_180);      // 180° rotation
            break;

        case LCD_ORIENT_LANDSCAPE:
            LcdStruct.dir    = 1;
            LcdStruct.width  = LCD_HEIGHT;
            LcdStruct.height = LCD_WIDTH;

            /* Landscape scan direction */
            LCD_Scan_Dir(LCD_DIR_90);     // 90° rotation
            break;

        default:
            return;   // Safety
    }
}

void LcdInitCommands(void)
{
	// Command 0x11: Sleep Out
	CommandBuffer = 0x11;
	LCD_Write(&CommandBuffer, 1, NULL, 0);

	delay_ms(120);                //ms

	// Command 0x36: Memory Access Control
	CommandBuffer = 0x36;
	DataBuffer [0] = 0x48;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0x3A: Interface Pixel Format
	CommandBuffer = 0x3A;
	DataBuffer [0] = 0x55;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xF0: Panel Driving Setting
	CommandBuffer = 0xF0;
	DataBuffer [0] = 0xC3;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xF0: Panel Driving Setting
	CommandBuffer = 0xF0;
	DataBuffer [0] = 0x96;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xB4: Display Inversion Control
	CommandBuffer = 0xB4;
	DataBuffer [0] = 0x01;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xB7: Entry Mode Set
	CommandBuffer = 0xB7;
	DataBuffer [0] = 0xC6;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xB9: Power Control 3
    CommandBuffer = 0xB9;
	DataBuffer [0] = 0x02;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xC0: Power Control 1
    CommandBuffer = 0xC0;
	DataBuffer [0] = 0xF0;
    DataBuffer [1] = 0x65;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 2);

	// Command 0xC1: Power Control 2
    CommandBuffer = 0xC1;
	DataBuffer [0] = 0x15;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xC2: VCOM Control
    CommandBuffer = 0xC2;
	DataBuffer [0] = 0xAF;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

    // Command 0x51: Brightness Control
    CommandBuffer = 0x51;
	DataBuffer [0] = 0xFF;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xC5: VCOM Control 2
    CommandBuffer = 0xC5;
	DataBuffer [0] = 0x22;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xE8: Positive Voltage Gamma Control
    CommandBuffer = 0xE8;
	DataBuffer [0] = 0x40;
    DataBuffer [1] = 0x8A;
    DataBuffer [2] = 0x00;
    DataBuffer [3] = 0x00;
    DataBuffer [4] = 0x29;
    DataBuffer [5] = 0x19;
    DataBuffer [6] = 0xA5;
    DataBuffer [7] = 0x33;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 8);

	// Command 0xE9: Negative Voltage Gamma Control
    CommandBuffer = 0xE0;
	DataBuffer [0] = 0xD0;
    DataBuffer [1] = 0x04;
    DataBuffer [2] = 0x08;
    DataBuffer [3] = 0x09;
    DataBuffer [4] = 0x08;
    DataBuffer [5] = 0x15;
    DataBuffer [6] = 0x2F;
    DataBuffer [7] = 0x42;
    DataBuffer [8] = 0x46;
    DataBuffer [9] = 0x28;
    DataBuffer [10] = 0x15;
    DataBuffer [11] = 0x16;
    DataBuffer [12] = 0x29;
    DataBuffer [13] = 0x2D;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 14);

	// Command 0xE1: Negative Voltage Gamma Control
    CommandBuffer = 0xE1;
	DataBuffer [0] = 0xD0;
    DataBuffer [1] = 0x04;
    DataBuffer [2] = 0x09;
    DataBuffer [3] = 0x09;
    DataBuffer [4] = 0x08;
    DataBuffer [5] = 0x15;
    DataBuffer [6] = 0x2E;
    DataBuffer [7] = 0x46;
    DataBuffer [8] = 0x46;
    DataBuffer [9] = 0x28;
    DataBuffer [10] = 0x15;
    DataBuffer [11] = 0x15;
    DataBuffer [12] = 0x29;
    DataBuffer [13] = 0x2D;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 14);

	// Command 0xF0: Panel Driving Setting
    CommandBuffer = 0xF0;
	DataBuffer [0] = 0x3C;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	// Command 0xF0: Panel Driving Setting
    CommandBuffer = 0xF0;
	DataBuffer [0] = 0x69;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 1);

	delay_ms(120);

	// Command 0x21: Display Inversion On
    CommandBuffer = 0x21;
    LCD_Write(&CommandBuffer, 1, NULL, 0);

	// Command 0x29: Display ON
    CommandBuffer = 0x29;
    LCD_Write(&CommandBuffer, 1, NULL, 0);
}

/*
 * @brief Initialize the SPI interface for the LCD
 * return ESP_OK on success, error code otherwise
 */
esp_err_t Spi_lcd_init(void)
{
    esp_err_t ret;

	// System SPI pins are usually pre-configured; skip reconfiguration
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL<<PIN_CS) | (1ULL<<PIN_DC) | (1ULL<<PIN_RST) | (1ULL<<LCD_PIN_BCKL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        // ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
		memset(debugbuf,0,sizeof(debugbuf));
		sprintf(debugbuf,"gpio_config failed: %s", esp_err_to_name(ret));
		uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
        return ret;
    }

	LCD_BCKL_EN;
    CS_HIGH();    // deassert CS
    DC_DATA();    // default data
    RST_HIGH();

    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_SPI_TX_SIZE,
    };

    // initialize the SPI bus (HSPI_HOST or VSPI_HOST)
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
		memset(debugbuf,0,sizeof(debugbuf));
		sprintf(debugbuf,"spi_bus_initialize failed: %s", esp_err_to_name(ret));
		uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
        return ret;
    }

    // Device interface configuration
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz
        .mode = 0,                          // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = -1,                 // NO automatic CS
        .queue_size = 1,                    // number of transactions in queue
        .flags = SPI_DEVICE_NO_DUMMY | SPI_DEVICE_HALFDUPLEX,
    };

    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev);
    if (ret != ESP_OK) {
		sprintf(debugbuf,"spi_bus_add_device failed: %s", esp_err_to_name(ret));
		uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
        return ret;
    }

    uart_write_bytes(UART_NUM_0, "SPI initialized at 20 MHz\r\n",sizeof("SPI initialized at 20 MHz\r\n"));
    return ESP_OK;
}

/*
 * @brief Hardware reset pulse
 */
void lcd_reset_pulse(void)
{
    RST_LOW();
    vTaskDelay(pdMS_TO_TICKS(20));
    RST_HIGH();
    vTaskDelay(pdMS_TO_TICKS(120));
}

/* =============================================================
   Fill the entire LCD with a single color
   color: color to fill
*/
#define BUF_H   40     // 20–40 is ideal

void LCD_Clear(u16 color)
{
    u32 index=0;
	u32 totalpoint= LcdStruct.width * LcdStruct.height;
	u32 pixels_per_chunk = LCD_SPI_TX_SIZE/2;

	/* Allocate DMA buffer locally */
    uint16_t *lcd_dma_buf = heap_caps_malloc(
        pixels_per_chunk * 2,
        MALLOC_CAP_DMA | MALLOC_CAP_8BIT
    );
	assert(lcd_dma_buf);

	// 1. Prepare DMA buffer with color
    for (uint32_t i = 0; i < pixels_per_chunk; i++)
    {
        lcd_dma_buf[i] = color;
    }
	// 2. Set window to full screen
    LCD_Set_Window(0,0,LcdStruct.width,LcdStruct.height);
	LCD_WriteRAM_Prepare();

	// 3. Send color data in chunks
	DC_DATA();
	LCD_CS_LO;

	for(index=0; index<totalpoint; index += pixels_per_chunk)
	{
		uint32_t remaining = totalpoint - index;
        uint32_t send_pixels = (remaining > pixels_per_chunk) ? pixels_per_chunk : remaining;
        Spi_SendBuffer((uint8_t*)lcd_dma_buf, send_pixels * 2);   // bytes
	}
	LCD_CS_HI;

    /* Free DMA buffer */
    heap_caps_free(lcd_dma_buf);
}

/* =============================================================
   Fill a rectangle with a single color
   (sx,sy),(ex,ey): rectangle corners
   color: fill color
*/
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{
	u16 i,j;
	u16 xlen=0;
	xlen=ex-sx+1;
	for(i=sy;i<=ey;i++)
	{
	 	LCD_SetCursor(sx,i);
		LCD_WriteRAM_Prepare();
		for(j=0;j<xlen;j++)
		{
			LCD_WriteRAM(color);
		}
	}
}

/* =============================================================
   Fill a rectangle with an array of colors
   (sx,sy),(ex,ey): rectangle corners
   color: pointer to color array
*/
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
{
	u16 height,width;
	u16 i,j;
	width=ex-sx+1;
	height=ey-sy+1;
 	for(i=0;i<height;i++)
	{
 		LCD_SetCursor(sx,sy+i);
		LCD_WriteRAM_Prepare();
		for(j=0;j<width;j++)
		{
			LCD_WriteRAM(color[i*height+j]);
		}
	}
}

void LCD_WriteBitmap(uint16_t x,
                     uint16_t y,
                     uint16_t w,
                     uint16_t h,
                     uint8_t *color_bytes)
{
    LCD_Set_Window(x, y, x + w, y + h);

    LCD_WriteRAM_Prepare();

    LCD_WR_DATA_BUF(color_bytes, (w * h) * 2);
}

/* =============================================================
   Draw a line using Bresenham's algorithm
   (x1,y1),(x2,y2): line endpoints
   color: line color
*/
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	u16 t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1;
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1;
	else if(delta_x==0)incx=0;
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;
	else{incy=-1;delta_y=-delta_y;}
	if( delta_x>delta_y)distance=delta_x;
	else distance=delta_y;
	for(t=0; t<=distance+1; t++ )
	{
		LCD_DrawPoint(uRow,uCol,color);
		xerr+=delta_x ;
		yerr+=delta_y ;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

/* =============================================================
   Draw a rectangle
   (x1,y1),(x2,y2): rectangle corners
   color: rectangle color
*/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	LCD_DrawLine(x1,y1,x2,y1,color);
	LCD_DrawLine(x1,y1,x1,y2,color);
	LCD_DrawLine(x1,y2,x2,y2,color);
	LCD_DrawLine(x2,y1,x2,y2,color);
}

/* =============================================================
   Draw a circle using Bresenham's algorithm
   (x0,y0): circle center
   r: radius
   color: circle color
*/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	int di;
	a=0;b=r;
	di=3-(r<<1);
	while(a<=b)
	{
		LCD_DrawPoint(x0+a,y0-b,color);             //5
 		LCD_DrawPoint(x0+b,y0-a,color);             //0
		LCD_DrawPoint(x0+b,y0+a,color);             //4
		LCD_DrawPoint(x0+a,y0+b,color);             //6
		LCD_DrawPoint(x0-a,y0+b,color);             //1
 		LCD_DrawPoint(x0-b,y0+a,color);
		LCD_DrawPoint(x0-a,y0-b,color);             //2
  		LCD_DrawPoint(x0-b,y0-a,color);             //7
		a++;

		if(di<0)
		{
			di +=4*a+6;
		}
		else
		{
			di+=10+4*(a-b);
			b--;
		}
	}
}

/* =============================================================
   Display a character at (x,y)
   x,y: starting coordinates
   chr: character to display
   size: font size (12 or 16)
   mode: 0=overwrite background, 1=transparent
   color: character color
*/
void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint16_t temp, t1, t;
    uint16_t y0 = y;
    uint16_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);  // 24/8*16=48 16/8*8=16 12/8*6=9 64/8*32=256

    switch (size)
    {
        case 12:
            chr = chr - ' ';
            pfont = (uint8_t *)asc2_1206[(uint8_t)chr]; /* font 1206 */
            break;

        case 16:
            chr = chr - ' ';
            pfont = (uint8_t *)asc2_1608[(uint8_t)chr];  /* font 1608 */
            break;

        case 24:
            chr = chr - ' ';
            pfont = (uint8_t *)arialBlack_24x16[(uint8_t)chr];  /* font 2416 */
            break;

        case 64:
            chr = chr - '0';
            pfont = (uint8_t *)arialBlack_64x32[(uint8_t)chr];  /* font 64x32 */
            break;

        default:
            return ;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];                                 /* font data */

        for (t1 = 0; t1 < 8; t1++)                       /* 8 bits per byte */
        {
            if (temp & 0x80)                             /* bit 7 is set */
            {
                LCD_DrawPoint(x, y, color);              /* draw pixel in character color */
            }
            else if (mode == 0)                          /* background mode */
            {
                LCD_DrawPoint(x, y, BACK_COLOR);         /* draw pixel in background color */
            }

            temp <<= 1;                                  /* shift to next bit */
            y++;

            if (y >= LcdStruct.height)return;               /* y out of bounds */

            if ((y - y0) == size)                        /* end of character column */
            {
                y = y0;                                  /* reset y to starting position */
                x++;                                     /* move to next column */

                if (x >= LcdStruct.width)return;            /* x out of bounds */

                break;
            }
        }
    }
}

/* =============================================================
   Display a character in various formats at (x,y)
   form: character format (FORM32X64, FORM48X64, FORM16X24)
   x,y: starting coordinates
   value: character to display
   dcolor: text color
   bgcolor: background color
*/
// void LCD_ShowCharEx(uint16_t x,
//                     uint16_t y,
//                     char value,
//                     font_t font,
//                     uint16_t fg,
//                     uint16_t bg)
// {
//     const uint8_t *font_ptr;
//     uint16_t w, h;
//     uint32_t bytes;

//     switch (font)
//     {
//         case FONT_32X64:
//             font_ptr = chlib;
//             w = 48; h = 96;
//             if (value == '-') font_ptr += 10 * 576;
//             else if (value == ' ') font_ptr += 11 * 576;
//             else font_ptr += (value - '0') * 576;
//             break;

//         case FONT_48X64:
//             font_ptr = chlib_48x64;
//             w = 48; h = 64;
//             if (value == '-') font_ptr += 10 * 384;
//             else if (value == ' ') font_ptr += 11 * 384;
//             else font_ptr += (value - '0') * 384;
//             break;

//         default:    // FONT_24X36
//             font_ptr = chlibsmall;
//             w = 24; h = 36;
//             font_ptr += (value - 32) * 108;
//             break;
//     }

//     bytes = (w * h) / 8;

//     /* Set drawing window */
//     LCD_Set_Window(x, y, w, h);
//     LCD_WriteRAM_Prepare();

//     // DC_DATA();
//     // LCD_CS_LO;

//     for (uint32_t j = 0; j < bytes; j++)
//     {
//         uint8_t data = *font_ptr++;

//         for (uint8_t i = 0; i < 8; i++)
//         {
//             if (data & 0x01)
//             {
//                 LCD_WriteRAM(fg);
//             }
//             else
//             {
//                 LCD_WriteRAM(bg);
//             }
//             data >>= 1;
//         }
//     }

//     // LCD_CS_HI;
// }


/* =============================================================
   Calculate m to the power of n
*/
u32 LCD_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;
	return result;
}

/* =============================================================
   Display a number at (x,y)
   x,y: starting coordinates
   num: number to display (0~999999999)
   len: number of digits to display
   size: font size
   color: character color
*/
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color)
{
	u8 t,temp;
	u8 enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0,color);
				continue;
			}else enshow=1;

		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0,color);
	}
}

/* =============================================================
   Display a number at (x,y) with leading zeros option
   x,y: starting coordinates
   num: number to display (0~999999999)
   len: number of digits to display
   size: font size
   mode: bit 7 = leading zeros if set, bit 0 = background mode if set
   color: character color
*/
void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode,u16 color)
{
	u8 t,temp;
	u8 enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01,color);
				else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01,color);
 				continue;
			}else enshow=1;

		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01,color);
	}
}

/* =============================================================
   Display a string at (x,y)
   x,y: starting coordinates
   width,height: area dimensions
   size: font size
   p: pointer to string
   color: character color
*/
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p,u16 color)
{
	u8 x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' ')) // loop until null terminator
    {
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//�˳�
        LCD_ShowChar(x,y,*p,size,0,color);
        x+=size/2;
        p++;
    }
}

/* =============================================================
   Set the drawing window
   (sx,sy): starting coordinates
   width,height: dimensions of the window
*/
void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{
	width=sx+width-1;
	height=sy+height-1;

	CommandBuffer = LcdStruct.setxcmd;
	DataBuffer [0] = sx >> 8;
	DataBuffer [1] = sx & 0xFF;
	DataBuffer [2] = width >> 8;
	DataBuffer [3] = width & 0xFF;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 4);

	CommandBuffer = LcdStruct.setycmd;
	DataBuffer [0] = sy >> 8;
	DataBuffer [1] = sy & 0xFF;
	DataBuffer [2] = height >> 8;
	DataBuffer [3] = height & 0xFF;
	LCD_Write(&CommandBuffer, 1, DataBuffer, 4);

}

void LCD_Init(void)
{
    esp_err_t r = Spi_lcd_init();
    if (r != ESP_OK) {
		uart_write_bytes(UART_NUM_0, "spi init failed", strlen("spi init failed"));
        return;
    }
    lcd_reset_pulse();
    LcdInitCommands();
    vTaskDelay(pdMS_TO_TICKS(120));
    uart_write_bytes(UART_NUM_0, "spi Clear Display", strlen("spi Clear Display"));
    LCD_Display_Dir(LCD_ORIENT_LANDSCAPE);  // 1: horizontal 0: vertical
	vTaskDelay(pdMS_TO_TICKS(50));
    LCD_Clear(LIGHTBLUE);
    Touch_Drv_Init();
    vTaskDelay(pdMS_TO_TICKS(50));
}


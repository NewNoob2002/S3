#include "Arduino.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp32-hal-ledc.h"
#include "esp_log.h"

#include "Begin.h"

#include "st7789.h"

#define TAG "ST7789"
#define _DEBUG_ 0
#define _USEDMA 0

#define LCD_SPI SPI2_HOST

#define SPI_DEFAULT_FREQUENCY SPI_MASTER_FREQ_20M;

int clock_speed_hz = SPI_DEFAULT_FREQUENCY;

spi_device_handle_t _SPIHandle;
TFT_t lcd_config;

void spi_clock_speed(int speed)
{
    ESP_LOGI(TAG, "SPI clock speed=%d MHz", speed / 1000000);
    clock_speed_hz = speed;
}

void spi_master_init(int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL)
{
    pinMode(GPIO_CS, OUTPUT);
    pinMode(GPIO_DC, OUTPUT);
    pinMode(GPIO_RESET, OUTPUT);
    ledcAttach(GPIO_BL, 8000, 12);

    digitalWrite(GPIO_CS, 0);
    digitalWrite(GPIO_DC, 0);

    digitalWrite(GPIO_RESET, 1);
    delay(100);
    digitalWrite(GPIO_RESET, 0);
    delay(100);
    digitalWrite(GPIO_RESET, 1);
    delay(100);

    ledcWrite(GPIO_BL, 0);

    ESP_LOGI(TAG, "GPIO_MOSI=%d", GPIO_MOSI);
    ESP_LOGI(TAG, "GPIO_SCLK=%d", GPIO_SCLK);
    spi_bus_config_t buscfg = {
        .mosi_io_num = (gpio_num_t)GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = (gpio_num_t)GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0};

    esp_err_t ret = spi_bus_initialize(LCD_SPI, &buscfg, SPI_DMA_CH_AUTO);
    ESP_LOGD(TAG, "spi_bus_initialize=%d", ret);
    assert(ret == ESP_OK);

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    // devcfg.command_bits = 8;
    // devcfg.clock_speed_hz = SPI_Frequency;
    devcfg.clock_speed_hz = clock_speed_hz;
    devcfg.queue_size = 7;
    // devcfg.mode = 2;
    devcfg.mode = 3;
    devcfg.flags = SPI_DEVICE_NO_DUMMY;

    if (GPIO_CS >= 0)
    {
        devcfg.spics_io_num = GPIO_CS;
    }
    else
    {
        devcfg.spics_io_num = -1;
    }

    ret = spi_bus_add_device(LCD_SPI, &devcfg, &_SPIHandle);
    ESP_LOGD(TAG, "spi_bus_add_device=%d", ret);
    assert(ret == ESP_OK);
    lcd_config._bl = GPIO_BL;
    lcd_config._dc = GPIO_DC;
}

bool spi_master_write_byte(const uint8_t *Data, size_t DataLength)
{
    spi_transaction_t SPITransaction;
    esp_err_t ret;

    if (DataLength > 0)
    {
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = DataLength * 8;
        SPITransaction.tx_buffer = Data;
#if 1
        ret = spi_device_transmit(_SPIHandle, &SPITransaction);
#else
        ret = spi_device_polling_transmit(SPIHandle, &SPITransaction);
#endif
        assert(ret == ESP_OK);
    }

    return true;
}

bool spi_master_write_byteDMA(const uint8_t *Data, size_t DataLength)
{
    if (DataLength % 3 != 0)
    {
        ESP_LOGE(TAG, "DataLength=%d, not match 32bit block", DataLength);
        return false;
    }
    spi_transaction_t SPITransaction;
    esp_err_t ret;
    uint32_t DMABuffSize = (sizeof(uint8_t) * DataLength);
    uint8_t *DMABuff = (uint8_t *)heap_caps_malloc(DMABuffSize, MALLOC_CAP_DMA);

    if (DMABuff == NULL)
    {
        ESP_LOGE(TAG, "DMABuff malloc fail");
        return false;
    }
    memcpy(DMABuff, Data, DMABuffSize);
    memset(&SPITransaction, 0, sizeof(spi_transaction_t));

    if (DataLength > 0)
    {
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = DataLength * 8;
        SPITransaction.tx_buffer = DMABuff;
#if 1
        ret = spi_device_transmit(_SPIHandle, &SPITransaction);
#else
        ret = spi_device_polling_transmit(SPIHandle, &SPITransaction);
#endif
        assert(ret == ESP_OK);
    }
    heap_caps_free(DMABuff);
    return true;
}

bool spi_master_write_command(uint8_t cmd)
{
    static uint8_t Byte = 0;
    Byte = cmd;
    digitalWrite(lcd_config._dc, 0);
    bool ret = false;
#if _USEDMA

    ret = spi_master_write_byteDMA(&Byte, 1);
#else
    ret = spi_master_write_byte(&Byte, 1);
#endif // DEBUG
    return ret;
}

bool spi_master_write_data_byte(uint8_t data)
{
    static uint8_t Byte = 0;
    Byte = data;
    digitalWrite(lcd_config._dc, 1);
    bool ret = false;
#if _USEDMA

    ret = spi_master_write_byteDMA(&Byte, 1);
#else
    ret = spi_master_write_byte(&Byte, 1);
#endif // DEBUG
    return ret;
}

bool spi_master_write_data_word(uint16_t data)
{
    static uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    digitalWrite(lcd_config._dc, 1);
    bool ret = false;
#if _USEDMA

    ret = spi_master_write_byteDMA(Byte, 2);
#else
    ret = spi_master_write_byte(Byte, 2);
#endif // DEBUG
    return ret;
}

bool spi_master_write_addr(uint16_t addr1, uint16_t addr2)
{
    static uint8_t Byte[4];
    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;
    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;
    digitalWrite(lcd_config._dc, 1);
    return spi_master_write_byte(Byte, 4);
}
bool spi_master_write_color(uint16_t color, uint16_t size)
{
    static uint8_t Byte[1024];
    int index = 0;
    for (int i = 0; i < size; i++)
    {
        Byte[index++] = (color >> 8) & 0xFF;
        Byte[index++] = color & 0xFF;
    }
    digitalWrite(lcd_config._dc, 1);
    return spi_master_write_byte(Byte, size * 2);
}
bool spi_master_write_colors(uint16_t *colors, uint16_t size)
{
    return false;
}
void lcd_init(int width, int height, int offsetx, int offsety)
{
    lcd_config._width = width;
    lcd_config._height = height;
    lcd_config._offsetx = offsetx;
    lcd_config._offsety = offsety;
    lcd_config._font_direction = DIRECTION0;
    lcd_config._font_fill = false;
    lcd_config._font_underline = false;

    spi_master_write_command(0x01);
    delay(150);

    spi_master_write_command(0x11);
    delay(150);

    spi_master_write_command(0x3A);
    spi_master_write_data_byte(0x55);
    delay(10);

    spi_master_write_command(0x36);
    spi_master_write_data_byte(0x00);
    delay(10);

    spi_master_write_command(0x2A);
    spi_master_write_data_byte(offsetx >> 8);
    spi_master_write_data_byte(offsetx & 0xFF);
    spi_master_write_data_byte((offsetx + width - 1) >> 8);
    spi_master_write_data_byte((offsetx + width - 1) & 0xFF);
    delay(10);

    spi_master_write_command(0x2B);
    spi_master_write_data_byte(offsety >> 8);
    spi_master_write_data_byte(offsety & 0xFF);
    spi_master_write_data_byte((offsety + height - 1) >> 8);
    spi_master_write_data_byte((offsety + height - 1) & 0xFF);
    delay(10);

    spi_master_write_command(0x21);
    delay(10);

    spi_master_write_command(0x13);
    delay(10);

    spi_master_write_command(0x29);
    delay(10);

    ledcWrite(lcd_config._bl, 4095);
}

void lcd_setBacklight(int value)
{
    ledcWrite(lcd_config._bl, value);
}

void lcdDrawPixel(int x, int y, uint16_t color)
{
    if (x >= lcd_config._width)
        return;
    if (y >= lcd_config._height)
        return;

    if (lcd_config._use_frame_buffer)
    {
       lcd_config._frame_buffer[y * lcd_config._width + x] = color;
    }
    else
    {
        uint16_t _x = x + dev->_offsetx;
        uint16_t _y = y + dev->_offsety;

        spi_master_write_command(dev, 0x2A); // set column(x) address
        spi_master_write_addr(dev, _x, _x);
        spi_master_write_command(dev, 0x2B); // set Page(y) address
        spi_master_write_addr(dev, _y, _y);
        spi_master_write_command(dev, 0x2C); // Memory Write
        // spi_master_write_data_word(dev, color);
        spi_master_write_colors(dev, &color, 1);
    }
}

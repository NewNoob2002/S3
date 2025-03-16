#include "Arduino.h"

#include "st7789.h"

#include "Begin.h"



SPIClass lcd;
void Begin_lcd()
{
    spi_master_init(lcd_mosi, lcd_clk, lcd_cs, lcd_dc, lcd_rst, lcd_blk);
    lcd_init(172, 320, 35, 0);
}

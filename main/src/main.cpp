#include "Arduino.h"

#include "Begin.h"
#include "st7789.h"

extern "C" void app_main()
{
    spi_master_init(lcd_mosi, lcd_clk, lcd_cs, lcd_dc, lcd_rst, lcd_blk);
    lcd_init(172, 320, 35, 0);
    static int i = 0;
    while (1)
    {

        if(i == 4095) i = 0;
        lcd_setBacklight(i++);
        printf("Backlight: %d\n", i);
        delay(10);
    }
    return;
    // Do your own thing
}
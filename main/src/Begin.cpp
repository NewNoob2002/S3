#include "Arduino.h"


#include "Begin.h"



SPIClass lcd;
void spi_init()
{
    pinMode(lcd_cs, OUTPUT);
    digitalWrite(lcd_cs, 0);

    pinMode(lcd_dc, OUTPUT);
    digitalWrite(lcd_dc, 0);

    pinMode(lcd_blk, OUTPUT);
    digitalWrite(lcd_dc, 0);

    pinMode(lcd_rst, OUTPUT);
    digitalWrite(lcd_rst, 1);
    delay(100);
    digitalWrite(lcd_rst, 0);
    delay(100);
    digitalWrite(lcd_rst, 1);
    delay(100);

    lcd.begin(lcd_clk, -1, lcd_mosi, -1);
    lcd.beginTransaction(SPISettings(10*100*100, MSBFIRST, SPI_MODE3));

}

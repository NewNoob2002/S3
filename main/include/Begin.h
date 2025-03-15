#pragma once
#define lcd_mosi  13
#define lcd_clk   12
#define lcd_cs    10
#define lcd_dc    11
#define lcd_blk   14
#define lcd_rst   1

#include "SPI.h"

extern SPIClass lcd;
void spi_init();
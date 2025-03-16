#include "Arduino.h"

#include "Begin.h"
#include "st7789.h"

extern "C" void app_main()
{
    Begin_lcd();
    printf("framebuffer %d\n", lcd_config._use_frame_buffer);
    while (1)
    {
        lcdFillScreen(RED);
        lcdDrawFinish();
        delay(1000);
        lcdFillScreen(GREEN);
        lcdDrawFinish();
        delay(1000);
        lcdFillScreen(BLUE);
        lcdDrawFinish();
        delay(1000);
    }
    return;
    // Do your own thing
}
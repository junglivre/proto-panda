#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.hpp"
#include <list>
#include <string>
#include <vector>
#include "tools/psrammap.hpp"

class OledIcon{
    public:
        OledIcon():width(0),height(0),icon(nullptr){};
        OledIcon(int w, int h, uint8_t* b):width(w),height(h),icon(b){};
        int width;
        int height;
        uint8_t *icon;
};

class Panda_SSD1306 : public Adafruit_SSD1306{
    public:
        Panda_SSD1306(uint8_t w, uint8_t h, TwoWire *twi = &Wire, int8_t rst_pin = -1, uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL):Adafruit_SSD1306(w, h, twi, rst_pin, clkDuring, clkAfter){};

        bool begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = 0, bool reset = true, bool periphBegin = true){
            if ((!buffer) && !(buffer = (uint8_t *)ps_malloc(WIDTH * ((HEIGHT + 7) / 8))))
                return false;
            return Adafruit_SSD1306::begin(switchvcc, i2caddr, reset, periphBegin);
        }
};


class OledScreen{
    public:
        static bool Start();
        static void DrawCircularProgress(int val, int max, const char *title);
        static void DrawProgressBar(int val, int max, const char *title);
        static void DrawWaitForPower(float volts);
        static void CriticalFail(const char *str);
        static void Clear();
        static void SetConsoleMode(bool enable);
        static void PrintConsole(const char *str);
        static void PrintError(const char *str);
        static void DrawPanelFaceToScreen(int x, int y, int scale=1);
        static void DrawIcon(int x, int y, int iconId);
        static int CreateIcon(std::vector<uint8_t> iconData, int width, int height);

        static void MarkPixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue);
    
        static Panda_SSD1306 display;
        static bool consoleMode;
        static PSRAMList<PSRAMString> lines;
        static uint8_t* DisplayFace[2];
        static uint8_t screenFlipId;

        static PSRAMVector<OledIcon> icons;
    private:
        static uint32_t swapTimer;
};
#ifndef Screen_h
#define Screen_h

#include <Arduino.h>

class Screen
{
public:
    Screen();
    void init();
    void printLevels();
    void printMessage(String str);
    void powerDown();
    String info1;
    String info2;
    int co2;
    float temp;
    float h2o;
    int bat;

private:
    void drawFrame();
    void printString(int16_t x, int16_t y, int16_t w, int16_t h, String str);
    int16_t _width, _height;
    int16_t _co2X, _co2Y;
    int16_t _co2Width, _co2Height;
    int16_t _tempWidth, _tempHeight;
    int16_t _tempX, _tempY;
    int16_t _h2oWidth, _h2oHeight;
    int16_t _h2oX, _h2oY;
    int16_t _batX, _batY;
    int16_t _batWidth, _batHeight;
    int16_t _infoX, _infoY, _infoY2;
    int16_t _infoWidth, _infoHeight;
    int _step;
};

#endif
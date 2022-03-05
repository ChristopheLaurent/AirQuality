#define LILYGO_T5_V213

#include <Adafruit_I2CDevice.h> // work around dependency issue (see https://community.platformio.org/t/adafruit-gfx-lib-will-not-build-any-more-pio-5/15776 )
#include <boards.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP

// FreeFonts from Adafruit_GFX
// #include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
// #include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include "Screen.h"
#include "smiley.h"

GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

Screen::Screen()
{
}

void Screen::init()
{
    _step = 0;
    SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
    display.init();
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
}

void Screen::powerDown()
{
  display.powerDown();
}

void Screen::printMessage(String str)
{
  log_d("fill screen white");
  display.fillScreen(GxEPD_WHITE);
  log_d("set cursor");
  display.setCursor(0, 0);
  log_d("print");
  display.print(str);
  log_d("update");
  display.update();
  log_d("fill screen white");
  display.fillScreen(GxEPD_WHITE);
}

void Screen::printLevels()
{
  if (_step == 0)
  {
    drawFrame();
  }

  display.setFont(&FreeSansBold24pt7b);
  printString(_co2X, _co2Y, _co2Width, _co2Height, String(co2));
  display.setFont(&FreeSansBold12pt7b);
  printString(_tempX, _tempY, _tempWidth, _tempHeight, String((int)temp));
  printString(_h2oX, _h2oY, _h2oWidth, _h2oHeight, String((int)h2o));
  display.setFont((GFXfont*)NULL);
  display.setCursor(_infoX, _infoY);
  display.print(info1);
  display.setCursor(_infoX, _infoY2);
  display.print(info2);

  int bitmap;
  bitmap = (co2 <= 600) ? 0 : (co2 <= 800) ? 1 : 2;

  int16_t bitmapX = 0;
  int16_t bitmapY = 110;
  int16_t bitmapSize = 122;

  display.drawBitmap(bitmapX, bitmapY, epd_bitmap_allArray[bitmap], bitmapSize, bitmapSize, GxEPD_BLACK);

  int rate = (co2 - 600) / 6; // 600 => 0, 1200 => 100;
  if (rate > 0)
  {
    int center = bitmapSize / 2;
    int count = 0;
    for (int x = 0; x < bitmapSize; x++)
    {
      for (int y = 0; y < bitmapSize; y++)
      {
        count += rate;
        if (count > 99)
        {
          int distx = x - center;
          int disty = y - center;
          int dist = distx * distx + disty * disty;
          if (dist > 1900)
          {
            display.drawPixel(x + bitmapX, y + bitmapY, GxEPD_BLACK);
          }
          count = 0;
        }
      }
    }
  }

  display.fillRect(_batX, _batY, (bat * _batWidth) / 100.0, _batHeight, GxEPD_BLACK);

  if (_step > 1)
  {
    display.updateWindow(0, 0, display.width(), display.height(), true);
  }
  else
  {
    display.update();
  }

  display.fillRect(_co2X, _co2Y, _co2Width, _co2Height, GxEPD_WHITE);
  display.fillRect(_tempX, _tempY, _tempWidth, _tempHeight, GxEPD_WHITE);
  display.fillRect(_h2oX, _h2oY, _h2oWidth, _h2oHeight, GxEPD_WHITE);
  display.fillRect(bitmapX, bitmapY, bitmapSize, bitmapSize, GxEPD_WHITE);
  display.fillRect(_batX, _batY, _batWidth, _batHeight, GxEPD_WHITE);
  display.fillRect(_infoX, _infoY, _infoWidth, _infoHeight, GxEPD_WHITE);

  _step++;
}

void Screen::drawFrame()
{
  _width = display.width();
  _height = display.height();
  _co2X = 0;
  _co2Y = 0;
  _co2Width = _width - 10;
  _co2Height = _height / 5;
  // display.drawRect(_co2X, _co2Y, _co2Width, _co2Height, GxEPD_BLACK); // CO2
  display.setCursor(_co2X + _co2Width - 20, _co2Y + _co2Height);
  display.println("PPM");

  _tempWidth = (_width / 2) - 20;
  _tempHeight = _height / 10;
  _tempX = _co2X;
  _tempY = _co2Y + _co2Height + 20;
  // display.drawRect(_tempX, _tempY, _tempWidth, _tempHeight, GxEPD_BLACK); // temp
  display.drawCircle(_tempX + _tempWidth + 1, _tempY + 1, 1, GxEPD_BLACK);
  display.setCursor(_tempX + _tempWidth + 3, _tempY + 2);
  display.println("C");

  _h2oWidth = _tempWidth;
  _h2oHeight = _height / 10;
  _h2oX = _tempX + _tempWidth + 10;
  _h2oY = _tempY;
  // display.drawRect(_h2oX, _h2oY, _h2oWidth, _h2oHeight, GxEPD_BLACK); // H2O
  display.setCursor(_h2oX + _h2oWidth + 1, _tempY + (_tempHeight / 2));
  display.println("%H");

  _batWidth = 16;
  _batHeight = 6;
  _batX = _width - _batWidth - 4;
  _batY = _height - _batHeight - 3;
  display.drawRect(_batX-2, _batY-2, _batWidth+4, _batHeight+4, GxEPD_BLACK);
  display.fillRect(_batX + _batWidth+2, _batY + (_batHeight/2)-1, 2, 2, GxEPD_BLACK);

  _infoX = 2;
  _infoHeight = 16;
  _infoY = _height - _infoHeight;
  _infoY2 = _height - (_infoHeight / 2);
  _infoWidth = (_batX - _infoX) - 3;
  // display.drawRect(_infoX, _infoY, _infoWidth, _infoHeight, GxEPD_BLACK); // for test
}

void Screen::printString(int16_t x, int16_t y, int16_t w, int16_t h, String str)
{
  int16_t xs, ys;
  uint16_t ws, hs;
  display.getTextBounds(str, 0, 0, &xs, &ys, &ws, &hs);
  ws += str.length() + 2; // hack
  display.setCursor(x + w - ws, y + h - 4);
  display.print(str);
}

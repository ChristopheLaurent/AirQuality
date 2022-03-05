#include "SdCard.h"

#include <SD.h>
#include <FS.h>
#define LILYGO_T5_V213
#include <boards.h>

SPIClass SDSPI(VSPI);

RTC_DATA_ATTR int recordFileNum = 0;

SdCard::SdCard(TimeKeeper *timeKeeper)
{
    _cardType = CARD_NONE;
    _cardSize = 0;
    _initOk = false;
    _timeKeeper = timeKeeper;
}

bool SdCard::init(String sensorId, bool newFile)
{
    log_d("init SD card, new file = %d", newFile);
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI);
    if (SD.begin(SDCARD_CS, SDSPI))
    {
        _cardType = SD.cardType();
    }

    if (_cardType == CARD_NONE)
    {
        log_e("no SD Card");
        return false;
    }

    _cardSize = SD.cardSize() / (1024); // KB
    if ( _cardSize < 100)
    {
        log_e("SD Card too small: %dKB", (int)_cardSize);
        return false;
    }

    String sensorPath = String("/") + sensorId;
    if (!SD.exists(sensorPath))
    {
        SD.mkdir(sensorPath);
    }

    File root = SD.open(sensorPath);
    if (!root)
    {
        log_e("Failed to open %s directory", sensorPath.c_str());
        return false;
    }

    if (!root.isDirectory())
    {
        log_e("%s is not a directory", sensorPath.c_str());
        return false;
    }

    const char* mode;
    if (newFile)
    {
        File file = root.openNextFile();
        int count=0;
        while (file)
        {
            if (!file.isDirectory())
            {
                count++;
            }
            file = root.openNextFile();
        }

        recordFileNum = count+1;
        mode = FILE_WRITE;
    }
    else
    {
        mode = FILE_APPEND;
    }

    _recordFileName = sensorPath + "/Record" + String(recordFileNum) + ".csv";
    log_d("Use file %s mode %s", _recordFileName.c_str(), mode);

    _recordFile = SD.open(_recordFileName, mode);
    if (!_recordFile)
    {
        log_e ("Unable to open %s", _recordFileName.c_str());
        return false;
    }

    log_d ("Write records on %s", _recordFileName.c_str());
    if (!newFile)
    {
        size_t size = _recordFile.println("TimeStamp [s], date, CO2 [ppm], Temperature [degC], H2O [%], Voltage [V]");
        _initOk = (size > 0); // always true indeed
    }
    else
    {
        _initOk = true;
    }
    _recordCount = 0;
    return _initOk;
}

bool SdCard::appendRecord(uint16_t co2, float temp, float h2o, float voltage)
{
    if (_initOk)
    {
        unsigned long elapsed = millis()/1000;
        char* dateTime = _timeKeeper->toDate(_timeKeeper->now());
        size_t size = _recordFile.printf("%lu, %s, %d, %f, %f, %f\n", elapsed, dateTime, (int)co2, temp, h2o, voltage);
        log_v ("Write %d bytes to SD card\n", (int)size);
        _recordCount++;
        if (_recordCount % 10 == 0)
        {
            _recordFile.flush();
            /* flush will print an error on serial if it failed, due to missing SD card
            for example, but there is no way to know it from application side.
            Such an event should be recorded somehow in SD class.
            see https://github.com/espressif/arduino-esp32/issues/6303
             */
            /* to do if we know that flush failed:

                log_e("card is missing, try to reopen the file...");
                _recordFile = SD.open(_recordFileName, "w+");
                if (!_recordFile)
                {
                    log_e ("Unable to reopen " + _recordFileName);
                    return false;
                }
            */
        }
    }
    return _initOk;
}

void SdCard::close()
{
    if (_recordFile)
    {
        log_e("closing %s", _recordFileName.c_str());
        _recordFile.close();
    }
}

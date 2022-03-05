#ifndef SdCard_h
#define SdCard_h

#include <Arduino.h>
#include <FS.h>
#include "TimeKeeper.h"

class SdCard
{
    public :
        SdCard(TimeKeeper *timeKeeper);
        bool init(String sensorId, bool newFile);
        bool appendRecord(uint16_t co2, float temp, float h2o, float voltage);
        void close();
    private :
        TimeKeeper *_timeKeeper;
        uint8_t _cardType;
        uint64_t _cardSize;
        bool _initOk;
        String _recordFileName;
        File _recordFile;
        int _recordCount;
};
#endif
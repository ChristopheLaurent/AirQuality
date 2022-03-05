#ifndef TimeKeeper_h
#define TimeKeeper_h

#include <Arduino.h>

class TimeKeeper
{
    public :
        TimeKeeper();
        void setTime(double timeStamp);
        double now();
        char* toDate(double timeStamp);
        char* toDate(double timeStamp, const char* format);
    private :
        char date[256];
};
#endif
#include <SPIFFS.h>
#include <sys/time.h>
#include "time.h"
#include "TimeKeeper.h"

const char* _defaultDateTimeFormat = "%d/%b/%Y %H:%M:%S";

TimeKeeper::TimeKeeper()
{
}

void TimeKeeper::setTime(double timeStamp) // seconds since epoch including ms as decimals
{
    time_t secondsSinceEpoch = (time_t)timeStamp;

    struct timeval now = { .tv_sec = secondsSinceEpoch }; // loss of precision, does not matter
    settimeofday(&now, NULL);
}

double TimeKeeper::now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double)(now.tv_sec); // + (now.tv_usec/1000000.0);
}

char* TimeKeeper::toDate(double timeStamp)
{
    return TimeKeeper::toDate(timeStamp, _defaultDateTimeFormat);
}

char* TimeKeeper::toDate(double timeStamp, const char* format)
{
    time_t time = (time_t)timeStamp;
    tm *time_date;
    time_date = gmtime(&time);

    strftime(date, sizeof(date), strlen(format) == 0 ? _defaultDateTimeFormat : format, time_date);

    return date;
}

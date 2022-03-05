#ifndef Sensor_h
#define Sensor_h

#define SENSOR_LOW_POWER 1
#define SENSOR_NORMAL 2
#define SENSOR_MOCKUP 3


struct Measure
{
    double timeStamp;
    uint16_t co2;
    float temperature;
    float humidity;
};

class Sensor
	{
	public:
		Sensor();
        bool init(int mode);
        bool measure(double timeStamp);
        String getErrorMessage();
        char* getSerial();
        int getMode();
        void changeMode(int mode);
        void powerDown();
        struct Measure last;
        struct Measure* getStack(int *numMeasures);
        int stackSpace();
    private:
        void push();
        int _mode;
        uint16_t _error;
        char _errorMessage[256];
        char _serial[13];
    };
    
#endif
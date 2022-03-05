#include <SPIFFS.h>
#include "Sensor.h"

#include <SensirionI2CScd4x.h>
#define STACK_CAPACITY 50

SensirionI2CScd4x scd4x;
RTC_DATA_ATTR int stackIndex = 0;
RTC_DATA_ATTR struct Measure stack[STACK_CAPACITY];

Sensor::Sensor()
{
}

void Sensor::powerDown()
{
  scd4x.powerDown();
}

bool Sensor::init(int mode)
{
  Wire.beginTransmission(0x62);
  uint8_t status = Wire.endTransmission();
  if (status == 0)
  {
    _mode = mode;
  }
  else
  {
    sprintf(_errorMessage, "Sensor not responding, wire error code = %d", status);
    log_e("%s", _errorMessage);
    _mode = SENSOR_MOCKUP;
  }

  if (_mode == SENSOR_MOCKUP)
  {
    strcpy(_serial, "MOCK");
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    for (int i=2; i<6; i++) sprintf(_serial+(2*i), "%02X", mac[i]);
    return true;
  }

  scd4x.begin(Wire);
  uint16_t dataReady = false;

  if (_mode == SENSOR_NORMAL)
  {
    // In high power mode, we call init only once but we don't know if the
    // sensor was in low power (measurement every 30s) or high power (measurement every 5s)
    // mode. So it is just safer to stop periodic measurement and restart it.
    scd4x.stopPeriodicMeasurement();
  }
  else
  {
    // In low power mode, we are in a cycle of power on / power off
    // but sensor needs to be initialized only for the very first power on.
    // In the case of very first power on, the sensor has never been initialized
    // so dataReady from getDataReadyStatus() should be false
    // hence we can perform initialization. 
    _error = scd4x.getDataReadyStatus(dataReady);
    if (_error)
    {
      strcpy(_errorMessage, "Error in getDataReadyStatus()");
      return false;
    }
  }

  if (dataReady)
  {
    // we know 2 things: We are in low power mode and this is not the very first
    // initialisation, so we should find the serial number from SPIFF
    File file = SPIFFS.open(F("/serial.txt"), "r");
    if (file)
    {
      size_t size = file.readBytesUntil('\n', _serial, sizeof(_serial)-1);
      _serial[size] = '\0';
      if (size != sizeof(_serial)-1)
      {
        sprintf(_errorMessage, "Wrong serial number from SPIFF = %s", _serial);
        return false;
      }
    }
    else
    {
      strcpy(_errorMessage, "File serial.txt missing on SPIFF => no serial number");
      strcpy(_serial, "XXXXXXXXXXXX");
      return false;
    }
  }
  else
  {
    // This is the very first initialization, get serial number and save it.
    uint16_t serial[3];
    _error = scd4x.getSerialNumber(serial[0], serial[1], serial[2]);
    if (_error)
    {
      strcpy(_errorMessage, "Error in getSerialNumber()");
      return false;
    }

    sprintf(_serial, "%04X%04X%04X", serial[0], serial[1], serial[2]);
    log_d ("initialization, serial number = %s\n", _serial);

    File file = SPIFFS.open(F("/serial.txt"), "w");
    if (file)
    {
        file.printf("%s\n", _serial);
        file.flush();
        file.close();
    }
    else
    {
      log_e ("unable to write serial on SPIFF");
    }

    if (_mode == SENSOR_LOW_POWER)
    {
      log_d("start low power periodic measurement every 30s");
      _error = scd4x.startLowPowerPeriodicMeasurement();
    }
    else
    {
      log_d("start periodic measurement every 5s");
      _error = scd4x.startPeriodicMeasurement();
    }

    if (_error)
    {
      strcpy(_errorMessage, "Error in startPeriodicMeasurement()");
      return false;
    }

    log_d("Waiting for data ready");
    for (int i=0; i < 60 && !dataReady; i++)
    {
      log_d("%d",i);
      _error = scd4x.getDataReadyStatus(dataReady);
      if (_error)
      {
        strcpy(_errorMessage, "Error in getDataReadyStatus()");
        return false;
      }
      delay(1000);
    }
    log_i("CO2 [ppm], Temp [degC], Humidity [%]");
  }
  return true;
}

void Sensor::changeMode(int mode)
{
  if (_mode != SENSOR_MOCKUP)
  {
    scd4x.stopPeriodicMeasurement();
    
    if (mode == SENSOR_LOW_POWER)
    {
      log_d("start low power periodic measurement every 30s");
      _error = scd4x.startLowPowerPeriodicMeasurement();
    }
    else
    {
      log_d("start periodic measurement every 5s");
      _error = scd4x.startPeriodicMeasurement();
    }

    if (!_error)
    {
      _mode = mode;
    }
  }
}

bool Sensor::measure(double timeStamp)
{
  last.timeStamp = timeStamp;

  if (_mode == SENSOR_MOCKUP)
  {
    int seed = (int)millis();
    last.co2 = (seed / 300) % 1500;
    last.temperature = (seed / 10000) % 30;
    last.humidity = (seed / 2000) % 100;
    push();
    return true;
  }

  _error = scd4x.readMeasurement(last.co2, last.temperature, last.humidity);
  if (_error)
  {
    strcpy(_errorMessage, "Error in readMeasurement()");
    return false;
  }
  else if (last.co2 == 0)
  {
    strcpy(_errorMessage, "Invalid sample detected, skipping.");
    return false;
  }

  push();
  return true;
}

void Sensor::push()
{
  if (stackIndex < STACK_CAPACITY)
  {
    stack[stackIndex++] = last;
    log_v("%d measures pushed to stack", stackIndex);
  }
  else
  {
    log_e("cannot stack more measures");
  }
}

int Sensor::stackSpace()
{
  return STACK_CAPACITY - stackIndex;
}

Measure* Sensor::getStack(int *numMeasures)
{
  *numMeasures = stackIndex;
  stackIndex = 0;
  return stack;
}

int Sensor::getMode()
{
  return _mode;
}

char* Sensor::getSerial()
{
  return _serial;
}

String Sensor::getErrorMessage()
{
  String str1 = String(_errorMessage);
  String str2 = ": ";
  errorToString(_error, _errorMessage, 256);
  String str3 = String(_errorMessage);
  return str1 + str2 + str3;
}

void scanI2c() {
  byte error, address;
  int nDevices;
  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      String msg = String("I2C device found at address 0x");
      if (address < 16) msg += "0";
      msg += String(address, HEX);
      Serial.println(msg);
      nDevices++;
    }
    else if (error == 4)
    {
      String msg = String("Unknow error at address 0x");
      if (address < 16) msg += "0";
      msg += String(address, HEX);
      Serial.println(msg);
    }
  }
  if (nDevices == 0)
  {
    Serial.println("No I2C devices found");
  }
}

#ifndef MOCK_LIBRARIES_H
#define MOCK_LIBRARIES_H

#include "Arduino.h"
#include <cstdint>
#include <string>

// Hardware definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define ONE_WIRE_BUS 14

// Pin definitions
#define PIN_COMPRESSOR 12
#define PIN_FAN 2
#define PIN_DEFROST_VALVE 16
#define PIN_SUMP_HEATER 0
#define PIN_COMPRESSOR_HEATER 21
#define PIN_WATER_PUMP 13
#define PIN_WATER_VALVE 21

// Temperature thresholds
#define DELTA_1 1.0
#define DELTA_2 2.0
#define DELTA_3 3.0
#define minSensorTemp -40.0
#define maxSensorTemp 110.0
#define startCoolantTemp 35.0
#define compressorHeaterTemp -5.0
#define sumpHeaterTemp 5.0
#define sumpSuctionTemp 5.0
#define waterTargetTemp 40.0
#define fanTargetTemp 70.0
#define heatedAtLeastOnceTemp 66.0
#define defrostTemp 65.0

// Timing definitions
#define errorTime 3600000 // 1 hour in milliseconds
#define compressorDelayTime 30000 // 30 seconds
#define heatingDelayTime 1200000 // 20 minutes

// SSD1306 definitions
#define SSD1306_SWITCHCAPVCC 0x2

// State definitions
#define maxStateIndex 9

// Type definitions
using String = std::string;

// Struct definitions
struct TEMPS {
    float waterIntake;
    float waterInject;
    float coolantIntake;
    float coolantInject;
    float airOutside;
    float airInside;
};

struct DEVICES {
    bool compressor;
    bool fan;
    bool defrostValve;
    bool sumpHeater;
    bool compressorHeater;
    bool waterPump;
};

struct ERRORS {
    bool compressorError;
    bool defrostError;
    bool t1Error;
    bool t2Error;
    bool t3Error;
    bool t4Error;
    bool t5Error;
    bool t6Error;
};

struct STATE {
    TEMPS temps;
    DEVICES devices;
    ERRORS errors;
    int millis;
};

// Mock Wire library
class TwoWire {
public:
    void begin() { printf("I2C initialized\n"); }
};

extern TwoWire Wire;

// Mock SPI library
class SPIClass {
public:
    void begin() { printf("SPI initialized\n"); }
};

extern SPIClass SPI;

// Mock Adafruit_GFX
class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : WIDTH(w), HEIGHT(h) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) {}
    size_t write(uint8_t c) { putchar(c); return 1; }
protected:
    int16_t WIDTH;
    int16_t HEIGHT;
};

// Mock Adafruit_SSD1306
class Adafruit_SSD1306 : public Adafruit_GFX {
public:
    Adafruit_SSD1306(int16_t w, int16_t h, TwoWire* wire, int8_t rst) 
        : Adafruit_GFX(w, h), _wire(wire), _rst(rst) {}
    
    bool begin(uint8_t switchvcc, uint8_t i2caddr) {
        printf("OLED Display initialized\n");
        return true;
    }
    
    void display() { printf("Display updated\n"); }
    void clearDisplay() { printf("Display cleared\n"); }
    void setTextSize(uint8_t s) {}
    void setTextColor(uint16_t c) {}
    void setCursor(int16_t x, int16_t y) {}
    void cp437(bool x) {}
    void print(const char* s) { printf("%s", s); }
    void print(float f) { printf("%f", f); }
    void print(int i) { printf("%d", i); }
    void println(const char* s) { printf("%s\n", s); }
    void println(float f) { printf("%f\n", f); }
    void println(int i) { printf("%d\n", i); }
    
private:
    TwoWire* _wire;
    int8_t _rst;
};

// Mock DeviceAddress
typedef uint8_t DeviceAddress[8];

// Mock OneWire
class OneWire {
public:
    OneWire(uint8_t pin) : _pin(pin) {}
private:
    uint8_t _pin;
};

// Mock DallasTemperature
class DallasTemperature {
public:
    DallasTemperature(OneWire* wire) : _wire(wire) {}
    void begin() { printf("Temperature sensors initialized\n"); }
    void setResolution(uint8_t* addr, uint8_t res) {}
    void requestTemperatures() {}
    float getTempC(uint8_t* addr) { return 25.0; } // Mock temperature
private:
    OneWire* _wire;
};

#endif

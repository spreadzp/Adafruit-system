#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <stdint.h>
#include <stdio.h>
#include <string>

// Mock Arduino functions and types
typedef uint8_t byte;
#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// Mock Serial class
class SerialClass {
public:
    void begin(unsigned long baud) { printf("Serial initialized at %lu baud\n", baud); }
    void println(const char* str) { printf("%s\n", str); }
    void println(const std::string& str) { printf("%s\n", str.c_str()); }
    void println(float val) { printf("%f\n", val); }
    void println(int val) { printf("%d\n", val); }
    void print(const char* str) { printf("%s", str); }
    void print(float val) { printf("%f", val); }
    void print(int val) { printf("%d", val); }
};

extern SerialClass Serial;

// Mock digital I/O functions
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

// Mock time functions
unsigned long millis();
void delay(unsigned long ms);

#define F(str) str

#endif

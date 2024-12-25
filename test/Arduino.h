#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stdbool.h>

// Basic Arduino types
typedef bool boolean;
typedef uint8_t byte;

// Arduino functions we need to mock
void delay(unsigned long ms) {}
unsigned long millis() { return 0; }

// Pin modes
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// Pin values
#define HIGH 0x1
#define LOW 0x0

// Mock digitalWrite function
void digitalWrite(uint8_t pin, uint8_t val) {}

#endif

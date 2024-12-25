#include "Arduino.h"
#include <chrono>
#include <thread>
#include <map>

SerialClass Serial;
static std::map<uint8_t, uint8_t> pinModes;
static std::map<uint8_t, uint8_t> pinStates;
static auto programStart = std::chrono::steady_clock::now();

void pinMode(uint8_t pin, uint8_t mode) {
    pinModes[pin] = mode;
    printf("Pin %d set to mode %d\n", pin, mode);
}

void digitalWrite(uint8_t pin, uint8_t val) {
    pinStates[pin] = val;
    printf("Pin %d set to %d\n", pin, val);
}

int digitalRead(uint8_t pin) {
    return pinStates[pin];
}

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - programStart).count();
}

void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

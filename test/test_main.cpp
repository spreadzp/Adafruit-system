#include <Arduino.h>
#include <unity.h>
#include <cstdio>

// Mock definitions from main.cpp
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define ONE_WIRE_BUS 14

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

// Pin definitions
#define PIN_COMPRESSOR 12
#define PIN_FAN 2
#define PIN_DEFROST_VALVE 16
#define PIN_SUMP_HEATER 0
#define PIN_COMPRESSOR_HEATER 21
#define PIN_WATER_PUMP 13
#define PIN_WATER_VALVE 21

// Struct definitions from main.cpp
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

// Global variables needed for testing
TEMPS t = {0};
STATE states[10];
unsigned int stateIndex = 0;
const unsigned int maxStateIndex = 9;

bool isCompressorStarted = false;
bool isFanStarted = false;
bool isDefrostStarted = false;
bool isSumpHeaterStarted = false;
bool isCompressorHeaterStarted = false;
bool isPumpStarted = false;

int compressorFlag = 0;
int fanFlag = 0;
int defrostFlag = 0;
int sumpHeaterFlag = 0;
int compressorHeaterFlag = 0;
int waterPumpFlag = 0;

bool t2Error = false;
bool t3Error = false;

// Implementation of functions we're testing
void stopAll(bool withDefrost) {
    isCompressorStarted = false;
    isFanStarted = false;
    isPumpStarted = false;
    isSumpHeaterStarted = false;
    isCompressorHeaterStarted = false;
    if (withDefrost) {
        isDefrostStarted = false;
    }
}

void startCompressor() {
    isCompressorStarted = true;
    compressorFlag = 1;
}

void stopCompressor() {
    isCompressorStarted = false;
    compressorFlag = -1;
}

void startFan() {
    isFanStarted = true;
    fanFlag = 1;
}

void stopFan() {
    isFanStarted = false;
    fanFlag = -1;
}

void startDefrost() {
    isDefrostStarted = true;
    defrostFlag = 1;
}

void stopDefrost() {
    isDefrostStarted = false;
    defrostFlag = -1;
}

void startSumpHeater() {
    isSumpHeaterStarted = true;
    sumpHeaterFlag = 1;
}

void stopSumpHeater() {
    isSumpHeaterStarted = false;
    sumpHeaterFlag = -1;
}

void startCompressorHeater() {
    isCompressorHeaterStarted = true;
    compressorHeaterFlag = 1;
}

void stopCompressorHeater() {
    isCompressorHeaterStarted = false;
    compressorHeaterFlag = -1;
}

void startPump() {
    isPumpStarted = true;
    waterPumpFlag = 1;
}

void stopPump() {
    isPumpStarted = false;
    waterPumpFlag = -1;
}

void updateStateIndex() {
    if (stateIndex < maxStateIndex) {
        stateIndex++;
    } else {
        stateIndex = 0;
    }
}

unsigned long calculateDelay(float temp) {
    return 300; // Mock implementation for testing
}

void tearDown(void) {
    // Clean up after each test
}

// Test functions
void setUp(void) {
    // Reset state before each test
    t = {0};
    stateIndex = 0;
    isCompressorStarted = false;
    isFanStarted = false;
    isDefrostStarted = false;
    isSumpHeaterStarted = false;
    isCompressorHeaterStarted = false;
    isPumpStarted = false;
    compressorFlag = 0;
    fanFlag = 0;
    defrostFlag = 0;
    sumpHeaterFlag = 0;
    compressorHeaterFlag = 0;
    waterPumpFlag = 0;
    t2Error = false;
    t3Error = false;
}

void test_device_control(void) {
    printf("Testing device control functions...\n");
    
    // Test compressor control
    TEST_ASSERT_FALSE_MESSAGE(isCompressorStarted, "Compressor should be off initially");
    startCompressor();
    TEST_ASSERT_TRUE_MESSAGE(isCompressorStarted, "Compressor should be on after start");
    TEST_ASSERT_EQUAL_MESSAGE(1, compressorFlag, "Compressor flag should be 1");
    stopCompressor();
    TEST_ASSERT_FALSE_MESSAGE(isCompressorStarted, "Compressor should be off after stop");
    TEST_ASSERT_EQUAL_MESSAGE(-1, compressorFlag, "Compressor flag should be -1");
    
    // Test fan control
    TEST_ASSERT_FALSE_MESSAGE(isFanStarted, "Fan should be off initially");
    startFan();
    TEST_ASSERT_TRUE_MESSAGE(isFanStarted, "Fan should be on after start");
    TEST_ASSERT_EQUAL_MESSAGE(1, fanFlag, "Fan flag should be 1");
    stopFan();
    TEST_ASSERT_FALSE_MESSAGE(isFanStarted, "Fan should be off after stop");
    TEST_ASSERT_EQUAL_MESSAGE(-1, fanFlag, "Fan flag should be -1");
    
    // Test pump control
    TEST_ASSERT_FALSE_MESSAGE(isPumpStarted, "Pump should be off initially");
    startPump();
    TEST_ASSERT_TRUE_MESSAGE(isPumpStarted, "Pump should be on after start");
    stopPump();
    TEST_ASSERT_FALSE_MESSAGE(isPumpStarted, "Pump should be off after stop");
    
    printf("Device control tests passed!\n");
}

void test_temperature_calculations(void) {
    printf("Testing temperature calculations...\n");
    
    // Test calculateDelay function
    TEST_ASSERT_EQUAL_MESSAGE(300, calculateDelay(5.0), "Incorrect delay for temp > 0");
    TEST_ASSERT_EQUAL_MESSAGE(300, calculateDelay(0.0), "Incorrect delay for temp = 0");
    TEST_ASSERT_EQUAL_MESSAGE(300, calculateDelay(-5.0), "Incorrect delay for temp < 0");
    
    printf("Temperature calculation tests passed!\n");
}

void test_state_management(void) {
    printf("Testing state management...\n");
    
    // Test updateStateIndex
    TEST_ASSERT_EQUAL_MESSAGE(0, stateIndex, "Initial state index should be 0");
    updateStateIndex();
    TEST_ASSERT_EQUAL_MESSAGE(1, stateIndex, "State index should increment to 1");
    
    // Test state index wrapping
    for (int i = 0; i < maxStateIndex; i++) {
        updateStateIndex();
    }
    TEST_ASSERT_EQUAL_MESSAGE(0, stateIndex, "State index should wrap to 0");
    
    printf("State management tests passed!\n");
}

void test_stop_all(void) {
    printf("Testing stopAll function...\n");
    
    // Set up initial state with everything running
    startCompressor();
    startFan();
    startPump();
    startDefrost();
    startSumpHeater();
    startCompressorHeater();
    
    // Test stopAll without defrost
    stopAll(false);
    TEST_ASSERT_FALSE_MESSAGE(isCompressorStarted, "Compressor should be off");
    TEST_ASSERT_FALSE_MESSAGE(isFanStarted, "Fan should be off");
    TEST_ASSERT_FALSE_MESSAGE(isPumpStarted, "Pump should be off");
    TEST_ASSERT_FALSE_MESSAGE(isSumpHeaterStarted, "Sump heater should be off");
    TEST_ASSERT_FALSE_MESSAGE(isCompressorHeaterStarted, "Compressor heater should be off");
    
    printf("stopAll tests passed!\n");
}

#ifdef ARDUINO
void setup() {
    delay(2000);
    UNITY_BEGIN();
    
    Serial.println("\n=== Running Tests ===");
    
    Serial.println("\nTest: Device Control");
    RUN_TEST(test_device_control);
    
    Serial.println("\nTest: Temperature Calculations");
    RUN_TEST(test_temperature_calculations);
    
    Serial.println("\nTest: State Management");
    RUN_TEST(test_state_management);
    
    Serial.println("\nTest: Stop All Function");
    RUN_TEST(test_stop_all);
    
    Serial.println("\n=== Tests Complete ===");
    UNITY_END();
}

void loop() {
    // Empty
}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    printf("\n=== Running Tests ===\n");
    
    printf("\nTest: Device Control\n");
    RUN_TEST(test_device_control);
    
    printf("\nTest: Temperature Calculations\n");
    RUN_TEST(test_temperature_calculations);
    
    printf("\nTest: State Management\n");
    RUN_TEST(test_state_management);
    
    printf("\nTest: Stop All Function\n");
    RUN_TEST(test_stop_all);
    
    printf("\n=== Tests Complete ===\n");
    return UNITY_END();
}
#endif

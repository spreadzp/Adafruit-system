#include <iostream>
#include <thread>
#include <chrono>
#include "Arduino.h"
#include "mock_libraries.h"

// Global instances of mocked libraries
TwoWire Wire;
SPIClass SPI;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Global variables from main.cpp
TEMPS t = {0};
STATE states[maxStateIndex];
unsigned int stateIndex = 0;
bool isCompressorStarted = false;
bool isFanStarted = false;
bool isDefrostStarted = false;
bool isSumpHeaterStarted = false;
bool isCompressorHeaterStarted = false;
bool isPumpStarted = false;

// Mock temperature sensor addresses
DeviceAddress waterIntakeSensor = {0x28, 0x53, 0x8E, 0x95, 0xF0, 0x01, 0x3C, 0x34};
DeviceAddress waterInjectSensor = {0x28, 0x8A, 0x3D, 0x95, 0xF0, 0xFF, 0x3C, 0x22};
DeviceAddress coolantIntakeSensor = {0x28, 0xA6, 0x93, 0x95, 0xF0, 0x01, 0x3C, 0x3D};
DeviceAddress coolantInjectSensor = {0x28, 0x66, 0xC6, 0x95, 0xF0, 0x01, 0x3C, 0xE5};
DeviceAddress outsideAirSensor = {0x28, 0x32, 0xBD, 0x56, 0xB5, 0x01, 0x3C, 0xBF};
DeviceAddress insideAirSensor = {0x28, 0x32, 0xBD, 0x56, 0xB5, 0x01, 0x3C, 0xBF};

// Timing variables
unsigned long compressorStartedTime = 0;
unsigned long compressorStoppedTime = 0;
unsigned long fanStartedTime = 0;
unsigned long fanStoppedTime = 0;
unsigned long defrostStartedTime = 0;
unsigned long defrostStoppedTime = 0;
unsigned long sumpHeaterStartedTime = 0;
unsigned long sumpHeaterStoppedTime = 0;
unsigned long compressorHeaterStartedTime = 0;
unsigned long compressorHeaterStoppedTime = 0;
unsigned long pumpStartedTime = 0;
unsigned long pumpStoppedTime = 0;

// Control flags
bool startIsFinished = false;
unsigned long targetDelay = 0;
bool stateHasChanged = true;
bool tempHasChanged = true;
float tempsSum = 0;
bool heatedAtLeastOnce = false;
bool drawSign = false;

// Error flags
bool compressorError = false;
bool defrostError = false;
bool t1Error = false;
bool t2Error = false;
bool t3Error = false;
bool t4Error = false;
bool t5Error = false;
bool t6Error = false;

// Pin control flags
int compressorFlag = 0;
int fanFlag = 0;
int defrostFlag = 0;
int sumpHeaterFlag = 0;
int compressorHeaterFlag = 0;
int waterPumpFlag = 0;
int waterValveFlag = 0;

String mode = "work";

// Function implementations
void updateStateIndex() {
    if (stateIndex < maxStateIndex) {
        stateIndex++;
    } else {
        stateIndex = 0;
    }
}

void stopAll(bool withDefrost = false) {
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
    if (isCompressorStarted) return;
    compressorFlag = 1;
    isCompressorStarted = true;
    compressorStartedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_COMPRESSOR, HIGH);
    std::cout << "Compressor started" << std::endl;
}

void stopCompressor() {
    compressorFlag = -1;
    isCompressorStarted = false;
    compressorStoppedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_COMPRESSOR, LOW);
    std::cout << "Compressor stopped" << std::endl;
}

void startFan() {
    if (isFanStarted || isDefrostStarted) return;
    fanFlag = 1;
    isFanStarted = true;
    fanStartedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_FAN, HIGH);
    std::cout << "Fan started" << std::endl;
}

void stopFan() {
    fanFlag = -1;
    isFanStarted = false;
    fanStoppedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_FAN, LOW);
    std::cout << "Fan stopped" << std::endl;
}

void startDefrost() {
    if (isDefrostStarted) return;
    defrostFlag = 1;
    isDefrostStarted = true;
    defrostStartedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_DEFROST_VALVE, HIGH);
    std::cout << "Defrost started" << std::endl;
}

void stopDefrost() {
    defrostFlag = -1;
    isDefrostStarted = false;
    defrostStoppedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_DEFROST_VALVE, LOW);
    std::cout << "Defrost stopped" << std::endl;
}

void startSumpHeater() {
    if (isSumpHeaterStarted) return;
    sumpHeaterFlag = 1;
    isSumpHeaterStarted = true;
    sumpHeaterStartedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_SUMP_HEATER, HIGH);
    std::cout << "Sump heater started" << std::endl;
}

void stopSumpHeater() {
    sumpHeaterFlag = -1;
    isSumpHeaterStarted = false;
    sumpHeaterStoppedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_SUMP_HEATER, LOW);
    std::cout << "Sump heater stopped" << std::endl;
}

void startPump() {
    if (isPumpStarted) return;
    waterPumpFlag = 1;
    isPumpStarted = true;
    pumpStartedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_WATER_PUMP, HIGH);
    std::cout << "Water pump started" << std::endl;
}

void stopPump() {
    waterPumpFlag = -1;
    isPumpStarted = false;
    pumpStoppedTime = millis();
    stateHasChanged = true;
    digitalWrite(PIN_WATER_PUMP, LOW);
    std::cout << "Water pump stopped" << std::endl;
}

void sumpHeaterCheck() {
    if (t.airOutside <= sumpHeaterTemp) {
        startSumpHeater();
    }
    if (t.airOutside >= sumpHeaterTemp + DELTA_2) {
        stopSumpHeater();
    }
}

void compressorControl() {
    if (t.waterInject <= waterTargetTemp) {
        startCompressor();
    }
    if (heatedAtLeastOnce && t.waterInject >= waterTargetTemp + DELTA_2 && !isDefrostStarted) {
        stopAll();
        startIsFinished = false;
    }
}

void checkCompressorError() {
    if (!isFanStarted) return;
    if (millis() - fanStartedTime >= errorTime) {
        if (heatedAtLeastOnce) {
            stopAll();
            compressorError = true;
        } else {
            heatedAtLeastOnce = true;
            startDefrost();
            fanStartedTime = millis();
            drawSign = true;
        }
    }
}

void checkDefrostError() {
    if (!isDefrostStarted) return;
    if (millis() - defrostStartedTime >= errorTime) {
        stopAll();
        compressorError = true;
    }
}

void waterPumpControl() {
    if (!isCompressorStarted) return;
    if (t.coolantInject >= waterTargetTemp) {
        startPump();
    }
    if (heatedAtLeastOnce && (t.coolantInject <= (waterTargetTemp - DELTA_2))) {
        stopPump();
        startIsFinished = false;
    }
}

void fanControl() {
    if (!isCompressorStarted) return;
    if (t.coolantInject >= heatedAtLeastOnceTemp) {
        heatedAtLeastOnce = true;
    }
    if (t.coolantInject >= fanTargetTemp) {
        drawSign = false;
        stopFan();
    }
    if (t.coolantInject <= (fanTargetTemp - DELTA_2)) {
        startFan();
    }
}

void defrostStartControl() {
    if (!isCompressorStarted || !heatedAtLeastOnce) return;
    if (isDefrostStarted) return;
    std::cout << "Check defrost " << (int)isDefrostStarted << std::endl;
    if (t.coolantInject <= defrostTemp) {
        stopFan();
        startDefrost();
        mode = "defrost";
    }
}

void defrostStopControl() {
    if (!isDefrostStarted) return;
    if (t.coolantIntake >= sumpSuctionTemp) {
        stopDefrost();
        startFan();
        heatedAtLeastOnce = false;
        mode = "work";
    }
}

void getAllTemps() {
    sensors.requestTemperatures();
    t.waterIntake = sensors.getTempC(waterIntakeSensor);
    t.waterInject = sensors.getTempC(waterInjectSensor);
    t.coolantIntake = sensors.getTempC(coolantIntakeSensor);
    t.coolantInject = sensors.getTempC(coolantInjectSensor);
    t.airOutside = sensors.getTempC(outsideAirSensor);
    t.airInside = sensors.getTempC(insideAirSensor);
    
    std::cout << "Temperatures:" << std::endl;
    std::cout << "Water Intake: " << t.waterIntake << "°C" << std::endl;
    std::cout << "Water Inject: " << t.waterInject << "°C" << std::endl;
    std::cout << "Coolant Intake: " << t.coolantIntake << "°C" << std::endl;
    std::cout << "Coolant Inject: " << t.coolantInject << "°C" << std::endl;
    std::cout << "Air Outside: " << t.airOutside << "°C" << std::endl;
    std::cout << "Air Inside: " << t.airInside << "°C" << std::endl;
    
    float newTempsSum = abs(t.waterIntake) + abs(t.waterInject) + abs(t.coolantIntake) + abs(t.coolantInject) + abs(t.airOutside);
    if (tempsSum != newTempsSum) tempHasChanged = true;
    tempsSum = newTempsSum;
}

void setup() {
    Serial.begin(115200);
    
    // Initialize display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.display();
    delay(200);
    
    // Initialize temperature sensors
    sensors.begin();
    sensors.setResolution(waterIntakeSensor, 8);
    sensors.setResolution(waterInjectSensor, 8);
    sensors.setResolution(coolantIntakeSensor, 8);
    sensors.setResolution(coolantInjectSensor, 8);
    sensors.setResolution(outsideAirSensor, 8);
    
    // Initialize pins
    pinMode(PIN_COMPRESSOR, OUTPUT);
    pinMode(PIN_FAN, OUTPUT);
    pinMode(PIN_DEFROST_VALVE, OUTPUT);
    pinMode(PIN_SUMP_HEATER, OUTPUT);
    pinMode(PIN_COMPRESSOR_HEATER, OUTPUT);
    pinMode(PIN_WATER_PUMP, OUTPUT);
    
    // Stop all devices initially
    stopAll(true);
    
    std::cout << "Setup complete" << std::endl;
}

void loop() {
    delay(700);
    
    getAllTemps();
    
    if (compressorError || defrostError || t1Error || t2Error || t3Error || t4Error || t5Error) {
        stopAll(true);
        std::cout << "Errors detected - stopping all devices" << std::endl;
        return;
    }
    
    if (t.waterInject >= waterTargetTemp) {
        stopAll();
        std::cout << "Water target temperature reached - stopping all devices" << std::endl;
        return;
    }
    
    // Main control logic
    sumpHeaterCheck();
    compressorControl();
    
    if (isCompressorStarted) {
        checkCompressorError();
        waterPumpControl();
        fanControl();
        defrostStartControl();
        defrostStopControl();
        checkDefrostError();
    }
}

int main() {
    setup();
    
    std::cout << "\nStarting main loop simulation...\n" << std::endl;
    
    // Simulate running for 30 seconds
    for (int i = 0; i < 30; i++) {
        loop();
        std::cout << "\nSimulation time: " << i << " seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "\nSimulation complete!" << std::endl;
    return 0;
}

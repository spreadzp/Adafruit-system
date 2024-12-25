#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define ONE_WIRE_BUS             14                                     // подключения DS18B20 на пине 8


#define DELTA_1            1.0                                // дельта 1
#define DELTA_2            2.0                                // дельта 2
#define DELTA_3            3.0                                // дельта 3


#define minSensorTemp          -40.0                               // мин. температура, нижний придел NTC ERR
#define maxSensorTemp          110.0                               // макс. температура, верхний придел NTC ERR
#define startCoolantTemp        35.0                               // стартовая прог-ма по нагнетанию переход в work без подогрева картера компрессора
#define compressorHeaterTemp    -5.0                               // включение нагревателя картера компрессора
#define sumpHeaterTemp           5.0                               // целевая температура наружного датчика воздух
#define sumpSuctionTemp          5.0                               // рабочая температура фреона всасывания выключение оттайки
#define waterTargetTemp         40.0                               // рабочая температура воды нагнетания
#define fanTargetTemp           70.0                               // рабочая температура фреона нагнетания
#define heatedAtLeastOnceTemp   66.0                               // рабочая температура первоначального выхода перед выходом в ошибку "контрольная точка" 76.0
#define defrostTemp             65.0                               // рабочая температура фреона нагнетания включения оттайки

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


#define maxStateIndex     9

STATE states[maxStateIndex];
unsigned int stateIndex = 0;


//номера пинов
#define compressor           12                                     //реле компрессора
#define fan                  2                                     //реле вентилятора испарителя
#define defrostValve         16                                     //реле клапана оттайки
#define sumpHeater           0                                     //реле подогрева поддона
#define compressorHeater      21                                     //реле подогрева картера компрессора
#define waterCirculationPump  13                                     //реле циркуляционного насоса
#define waterPump             21                                     //реле клапана
#define waterValve          21                                     //реле клапана



#define errorTime              3600000 //10 min                 //период  ошибки, в милисекундах
#define compressorDelayTime     30000 //5 min                  стартовая прог-ма отложеный старт по нагнетанию (5мин. = 300000)
#define heatingDelayTime       1200000 //20 min                 стартовая прог-ма отложеный старт по наружной температуре ниже -5,вкл. нагрева картера компрессора

String mode = "work";


void stopAll(bool withDefrost=false);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress waterIntakeSensor = {
        0x28, 0x53, 0x8E, 0x95, 0xF0, 0x01, 0x3C, 0x34        // адрес датчика T1
};
DeviceAddress waterInjectSensor = {
        0x28, 0x8A, 0x3D, 0x95, 0xF0, 0xFF, 0x3C, 0x22         // адрес датчика T2
};
DeviceAddress coolantIntakeSensor = {
        0x28, 0xA6, 0x93, 0x95, 0xF0, 0x01, 0x3C, 0x3D         // адрес датчика T3
};
DeviceAddress coolantInjectSensor = {
        0x28, 0x66, 0xC6, 0x95, 0xF0, 0x01, 0x3C, 0xE5        // адрес датчика T4
};
DeviceAddress outsideAirSensor = {
        0x28, 0x32, 0xBD, 0x56, 0xB5, 0x01, 0x3C, 0xBF         // адрес датчика T5
};
DeviceAddress insideAirSensor = {
        0x28, 0x32, 0xBD, 0x56, 0xB5, 0x01, 0x3C, 0xBF        // адрес датчика T6
};

bool isCompressorStarted = false;
unsigned long compressorStartedTime = millis();
unsigned long compressorStoppedTime = millis();

bool isFanStarted = false;
unsigned long fanStartedTime = millis();
unsigned long fanStoppedTime = millis();

bool isDefrostStarted = false;
unsigned long defrostStartedTime = millis();
unsigned long defrostStoppedTime = millis();

bool isSumpHeaterStarted = false;
unsigned long sumpHeaterStartedTime = millis();
unsigned long sumpHeaterStoppedTime = millis();

bool isCompressorHeaterStarted = true;
unsigned long compressorHeaterStartedTime = millis();
unsigned long compressorHeaterStoppedTime = millis();

bool isPumpStarted = false;
unsigned long pumpStartedTime = millis();
unsigned long pumpStoppedTime = millis();


bool startIsFinished = false;
unsigned long targetDelay = 0;


bool stateHasChanged = true;
bool tempHasChanged = true;
float tempsSum = 0;

bool compressorError = false;
bool defrostError = false;
bool t1Error = false;
bool t2Error = false;
bool t3Error = false;
bool t4Error = false;
bool t5Error = false;
bool t6Error = false;

bool heatedAtLeastOnce = false;
bool drawSign = false;

//намерения на влкючения
int compressorFlag = 0; //1 - надо включить, -1 - надо выключить, 0 - ничего не надо делать
int fanFlag = 0;
int defrostFlag = 0;
int sumpHeaterFlag = 0;
int compressorHeaterFlag = 0;
int waterPumpFlag = 0;
int waterValveFlag = 0;

TEMPS t;

void setup() {
    Serial.begin(115200);

    //   SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        //        for (;;); // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
      display.display();
      delay(200); // Pause for 2 seconds

    // Wire.begin();
    sensors.begin();
    sensors.setResolution(waterInjectSensor, 8);
    sensors.setResolution(coolantIntakeSensor, 8);
    sensors.setResolution(coolantInjectSensor, 8);
    sensors.setResolution(outsideAirSensor, 8);
    // sensors.setResolution(insideAirSensor, 8);



    pinMode(compressor, OUTPUT);              // пин вкл/выкл. реле компрессора У1
    pinMode(fan, OUTPUT);              // пин вкл/выкл. реле вентилятора У2
    pinMode(defrostValve, OUTPUT);              // пин вкл/выкл. реле клапана оттайки У3
    pinMode(sumpHeater, OUTPUT);              // пин вкл/выкл. реле подогрева картера У4
    pinMode(compressorHeater, OUTPUT);              // пин вкл/выкл. реле подогрева компрессора У5
    pinMode(waterPump, OUTPUT);              // пин вкл/выкл. реле водяного насоса У6
    pinMode(waterValve, OUTPUT);
    // пин вкл/выкл. реле водяного клапана У7
    stopAll(true);
    switchPins();
    delay(1000);



}

void loop() {

    delay(700);

    t = getAllTemps();

//    saveState();

    if (compressorError || defrostError || t1Error || t2Error || t3Error || t4Error || t5Error) {
        stopAll(true);
        Serial.println("DrawErrors");
        drawErrors();
        return;
    }

    reDrawScreen();
    if(t.waterInject >= waterTargetTemp) {
        //TODO add something on screen later;
        stopAll();
        return;
    }
    start(t.coolantInject, t.airOutside);


    sumpHeaterCheck();              // нужна ли оттайка
    compressorControl();            // управление компрессором
    if (isCompressorStarted) {
        checkCompressorError();
        waterPumpControl();             // управление водяным насосом
        fanControl();                   // управление вентилятором
        defrostStartControl();               // управление оттайкой
        defrostStopControl();               // управление оттайкой
        checkDefrostError();
    }

//    switchPins();
}


void sumpHeaterCheck() {
    if (t.airOutside <= sumpHeaterTemp ) {
        startSumpHeater();
    }

    if (t.airOutside >= sumpHeaterTemp + DELTA_2) {
        stopSumpHeater();
    }
}

void compressorControl() {

    if (t.waterInject <= waterTargetTemp ) {
        startCompressor();
    }

    if (heatedAtLeastOnce && t.waterInject >= waterTargetTemp + DELTA_2 && !isDefrostStarted) {
        stopAll();
        startIsFinished = false;
    }
}

void checkCompressorError() {
    if (!isFanStarted) return;
    if (millis() - fanStartedTime >= errorTime ) {
        if(heatedAtLeastOnce) {
            stopAll();
            compressorError = true;
        } else {
            heatedAtLeastOnce = true;
            startDefrost();
            fanStartedTime = millis();
            drawSign= true;
        }
    }
}


void checkDefrostError() {
    if (!isDefrostStarted)  return;
    if (millis() - defrostStartedTime >= errorTime ) {
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
        drawSign= false;
        stopFan();
    }
    if (t.coolantInject <= (fanTargetTemp - DELTA_2)) {
        startFan();
    }
}

void defrostStartControl() {
    if (!isCompressorStarted || !heatedAtLeastOnce) return;
    if (isDefrostStarted) return;
//    Serial.print('Check defrost ');
    Serial.println((int) isDefrostStarted);

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


void stopAll(bool withDefrost) {
    stopCompressor();
    stopFan();
    stopPump();
    stopSumpHeater();
    stopCompressorHeater();
    if(withDefrost)stopDefrost();
}

void startCompressor() {
    if (isCompressorStarted) return;
//     digitalWrite(compressor, LOW);
    compressorFlag = 1;
    isCompressorStarted = true;
    compressorStartedTime = millis();
    stateHasChanged = true;
}

void stopCompressor() {
    // if (!isCompressorStarted) return;
//     digitalWrite(compressor, HIGH);
    compressorFlag = -1;
    isCompressorStarted = false;
    compressorStoppedTime = millis();
    stateHasChanged = true;
}

void startFan() {
    if (isFanStarted || isDefrostStarted) return;
//     digitalWrite(fan, LOW);
    fanFlag = 1;
    isFanStarted = true;
    fanStartedTime = millis();
    stateHasChanged = true;
}

void stopFan() {
    // if (!isFanStarted) return;
//     digitalWrite(fan, HIGH);
    fanFlag = -1;
    isFanStarted = false;
    fanStoppedTime = millis();
    stateHasChanged = true;
}

void startDefrost() {
    if (isDefrostStarted) return;
//     digitalWrite(defrostValve, LOW);
    defrostFlag = 1;
    isDefrostStarted = true;
    defrostStartedTime = millis();
    stateHasChanged = true;
}

void stopDefrost() {
    // if (!isDefrostStarted) return;
//     digitalWrite(defrostValve, HIGH);
    defrostFlag = -1;
    isDefrostStarted = false;
    defrostStoppedTime = millis();
    stateHasChanged = true;
}

void startSumpHeater() {
    if (isSumpHeaterStarted) return;
//     digitalWrite(sumpHeater, LOW);
    sumpHeaterFlag = 1;
    isSumpHeaterStarted = true;
    sumpHeaterStartedTime = millis();
    stateHasChanged = true;
}

void stopSumpHeater() {
    // if (!isSumpHeaterStarted) return;
//     digitalWrite(sumpHeater, HIGH);
    sumpHeaterFlag = -1;
    isSumpHeaterStarted = false;
    sumpHeaterStoppedTime = millis();
    stateHasChanged = true;
}

void startCompressorHeater() {
    if (isCompressorHeaterStarted) return;
//     digitalWrite(compressorHeater, LOW);
    compressorHeaterFlag = 1;
    isCompressorHeaterStarted = true;
    compressorHeaterStartedTime = millis();
    stateHasChanged = true;
}

void stopCompressorHeater() {
    // if (!isCompressorHeaterStarted) return;
//     digitalWrite(compressorHeater, HIGH);
    compressorHeaterFlag = -1;
    isCompressorHeaterStarted = false;
    compressorHeaterStoppedTime = millis();
    stateHasChanged = true;
    Serial.println("stopped");
}

void startPump() {
    if (isPumpStarted) return;
//     digitalWrite(waterPump, LOW);
    waterPumpFlag = 1;
    isPumpStarted = true;
    pumpStartedTime = millis();
    stateHasChanged = true;
}

void stopPump() {
    // if (isPumpStarted) return;
//     digitalWrite(waterPump, HIGH);
    waterPumpFlag = -1;
    isPumpStarted = false;
    pumpStoppedTime = millis();
    stateHasChanged = true;
}

void switchPins() {
    switchCompressorPin();
    switchFanPin();
    switchDefrostPin();
    switchCompressorHeaterPin();
    switchSumpHeaterPin();
    switchWaterPumpPin();
}

void switchCompressorPin() {
    switch (compressorFlag) {
        case 1:
            digitalWrite(compressor, LOW);
            break;
        case -1:
            digitalWrite(compressor, HIGH);
            break;
        default:
            break;
    }
    compressorFlag = 0;
}


void switchFanPin() {
    switch (fanFlag) {
        case 1:
            digitalWrite(fan, LOW);
            break;
        case -1:
            digitalWrite(fan, HIGH);
            break;
        default:
            break;
    }
    fanFlag = 0;
}


void switchDefrostPin() {
    switch (defrostFlag) {
        case 1:
            digitalWrite(defrostValve, LOW);
            break;
        case -1:
            digitalWrite(defrostValve, HIGH);
            break;
        default:
            break;
    }
    defrostFlag = 0;
}

void switchCompressorHeaterPin() {
    switch (compressorHeaterFlag) {
        case 1:
            digitalWrite(compressorHeater, LOW);
            break;
        case -1:
            digitalWrite(compressorHeater, HIGH);
            break;
        default:
            break;
    }
    compressorHeaterFlag = 0;
}


void switchSumpHeaterPin() {
    switch (sumpHeaterFlag) {
        case 1:
            digitalWrite(sumpHeater, LOW);
            break;
        case -1:
            digitalWrite(sumpHeater, HIGH);
            break;
        default:
            break;
    }
    sumpHeaterFlag = 0;
}


void switchWaterPumpPin() {
    switch (waterPumpFlag) {
        case 1:
            digitalWrite(waterPump, LOW);
            break;
        case -1:
            digitalWrite(waterPump, HIGH);
            break;
        default:
            break;
    }
    waterPumpFlag = 0;
}




void reDrawScreen() {


    if (!stateHasChanged && !tempHasChanged) return;
    tempHasChanged = false;
    stateHasChanged = false;

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.cp437(true);


    drawRelaysState();
    display.setTextSize(1);
    drawTemps();
    if (drawSign) {
        display.setTextSize(2);
        display.setCursor(4, 26);
        display.println("!");
    }

    display.display();
}


void drawRelaysState() {
    display.setTextSize(2);
    if (isCompressorStarted) {
        display.setCursor(1, 0);
        display.print("C");
    }
    if (isFanStarted) {
        display.setCursor(14, 0);
        display.print("F");
    }
    if (isDefrostStarted) {
        display.setCursor(27, 0);
        display.print("D");
    }
    if (isPumpStarted) {
        display.setCursor(40, 0);
        display.print("P");
    }
    display.setTextSize(0);
    if (isSumpHeaterStarted) {
        display.setCursor(53, 0);
        display.print("SH");
    }
    if (isCompressorHeaterStarted) {
        display.setCursor(53, 16);
        display.print("CH");
    }
}

void drawTemp(String text, float temp, int x, int y) {
    display.setCursor(x, y);

    Serial.print(text);
    Serial.println(temp);
    display.print(text);
    display.println(temp);


}
void drawTemps() {


    display.setTextSize(1);

    // drawTemp("T1:", t.waterIntake, 1, 29);

    drawTemp("T2:", t.waterInject, 1, 51);
//    Serial.print("T3:");Serial.println(t.coolantIntake);
    drawTemp("T3:", t.coolantIntake, 70, 29);

    drawTemp("T4:", t.coolantInject, 70, 51);

    drawTemp("T5:", t.airOutside, 70, 5);

    // drawTemp("T6:", t.airInside, 91, 51);

}

TEMPS getAllTemps() {

    sensors.requestTemperatures();

    TEMPS temps = {
//         5.0,5.0,5.0,5.0,5.0,5.0,
        sensors.getTempC(waterIntakeSensor),
        sensors.getTempC(waterInjectSensor),
        sensors.getTempC(coolantIntakeSensor),
        sensors.getTempC(coolantInjectSensor),
        sensors.getTempC(outsideAirSensor),
        sensors.getTempC(insideAirSensor),
    };

    // Serial.println(temps.waterIntake);
    Serial.println(temps.waterInject);
    Serial.println(temps.coolantIntake);
    Serial.println(temps.coolantInject);
    Serial.println(temps.airOutside);
    Serial.print("Compressor ");Serial.println((int)isCompressorStarted);
    Serial.print("Fan ");Serial.println((int)isFanStarted);
    Serial.print("Defrost ");Serial.println((int)isDefrostStarted);
    Serial.print("SumpHeater ");Serial.println((int)isSumpHeaterStarted);
    Serial.print("CompressorHeater ");Serial.println((int)isCompressorHeaterStarted);
    Serial.print("Pump ");Serial.println((int)isPumpStarted);
    Serial.println();

    // if(temps.waterIntake   < minSensorTemp || temps.waterIntake   > maxSensorTemp) t1Error = true;// else t1Error = false;
    if (temps.waterInject   < minSensorTemp || temps.waterInject   > maxSensorTemp) t2Error = true; // else t1Error = false;
    if (temps.coolantIntake < minSensorTemp || temps.coolantIntake > maxSensorTemp) t3Error = true; // else t1Error = false;
    if (temps.coolantInject < minSensorTemp || temps.coolantInject > maxSensorTemp) t4Error = true; // else t1Error = false;
    if (temps.airOutside    < minSensorTemp || temps.airOutside    > maxSensorTemp) t5Error = true; // else t1Error = false;
    // if(temps.airInside     < minSensorTemp || temps.airInside     > maxSensorTemp) t6Error = true;// else t1Error = false;

    float newTempsSum =  abs(temps.waterIntake) + abs(temps.waterInject) + abs(temps.coolantIntake) + abs(temps.coolantInject) + abs(temps.airOutside);
    if (tempsSum != newTempsSum) tempHasChanged = true;
    tempsSum = newTempsSum;

    return temps;
}

void drawText(String text, int x = 0, int y = 0) {
    display.setCursor(x, y);
    display.print(text);

}

void drawErrors() {

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.cp437(true);

    drawText("Error:");
    if (t1Error) {
        drawText("T1",  0, 22);
    }
    if (t2Error) {
        drawText("T2", 40, 22);
    }
    if (t3Error) {
        drawText("T3", 80, 22);
    }
    if (t4Error) {
        drawText("T4",  0, 44);
    }
    if (t5Error) {
        drawText("T5", 40, 44);
    }
    if (compressorError) {
        drawText("C", 80, 44);
    }
    if (defrostError) {
        drawText("D", 110, 44);
    }


    display.display();
}

void drawStart(float coolantInjectTemp, float airOutsideTemp ) {
    display.clearDisplay();
    unsigned int delaySeconds = (targetDelay - millis())/1000;
    unsigned int totalDelayMinutes = targetDelay/1000/60;

    display.setTextColor(SSD1306_WHITE);
    display.cp437(true);

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("Starting delay ");
    display.print(totalDelayMinutes);
    display.println("m:");

    display.setTextSize(1);
    if (isSumpHeaterStarted) drawText("SH", 90, 22);
    if (isCompressorHeaterStarted) drawText("CH", 90, 32);


    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(delaySeconds);
    display.println('s');

    display.setTextSize(1);
    drawTemp("T4:", coolantInjectTemp, 0, 42);
    drawTemp("T5:", airOutsideTemp, 0, 55);

    delay(100);

    display.display();
}

void start(float coolantInjectTemp, float airOutsideTemp) {
    if (startIsFinished) return;

    if (coolantInjectTemp >= startCoolantTemp ) {   //T4 >= 35
        stopAll();
    }

    targetDelay = millis() + calculateDelay(airOutsideTemp);

    while (millis() <= targetDelay) {
        sensors.requestTemperatures();
        airOutsideTemp = sensors.getTempC(outsideAirSensor);
        drawStart(sensors.getTempC(coolantInjectSensor), airOutsideTemp);

        if (airOutsideTemp <= sumpHeaterTemp) {
            startSumpHeater();
        }
        if (airOutsideTemp <= compressorHeaterTemp) {
            startCompressorHeater();
        }
        if (airOutsideTemp >= sumpHeaterTemp + DELTA_1) {
            stopSumpHeater();
            stopCompressorHeater();
        }
    }

    startIsFinished = true;

}

unsigned long calculateDelay(float temp) {
     return 300;
    return compressorDelayTime;

    int intTemp = (int)temp;
    if(temp >= 0.0) {
        Serial.print(temp);
        Serial.print(" delay is ");
        Serial.println(compressorDelayTime);
        return compressorDelayTime;
    }


    Serial.print(temp);
    Serial.print(" delay is ");
    Serial.println((unsigned long)((15.0 - temp) * 60000.0));
    return (unsigned long)((15.0 - temp) * 60000.0);
}

void saveState() {

    DEVICES devices = {
        isCompressorStarted,
        isFanStarted,
        isDefrostStarted,
        isSumpHeaterStarted,
        isCompressorHeaterStarted,
        isPumpStarted,
    };

    ERRORS errors = {
        compressorError,
        defrostError,
        t1Error,
        t2Error,
        t3Error,
        t4Error,
        t5Error,
        t6Error,
    };

    states[stateIndex] = {
        t,
        devices,
        errors,
        millis(),
    };
    updateStateIndex();
}

void updateStateIndex() {
    if (stateIndex < maxStateIndex) {
        stateIndex++;
    } else {
        stateIndex = 0;
    }
}
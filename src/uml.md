# UML Sequence Diagram

```mermaid
sequenceDiagram
    participant Main
    participant Sensors
    participant Display
    participant Compressor
    participant Fan
    participant WaterPump
    participant SumpHeater
    participant DefrostValve

    Main->>Sensors: getAllTemps()
    Sensors-->>Main: TEMPS {waterIntake, waterInject, coolantIntake, coolantInject, airOutside}
    
    alt airOutside ≤ 5.0°C
        Main->>SumpHeater: startSumpHeater()
        SumpHeater-->>Main: isSumpHeaterStarted = true
    else airOutside ≥ 7.0°C
        Main->>SumpHeater: stopSumpHeater()
        SumpHeater-->>Main: isSumpHeaterStarted = false
    end

    alt waterInject ≤ 40.0°C
        Main->>Compressor: startCompressor()
        Compressor-->>Main: isCompressorStarted = true
        
        alt coolantInject ≥ 40.0°C
            Main->>WaterPump: startPump()
            WaterPump-->>Main: isPumpStarted = true
        end
        
        alt coolantInject ≥ 66.0°C
            Main->>Main: heatedAtLeastOnce = true
        end
        
        alt coolantInject ≥ 70.0°C
            Main->>Fan: stopFan()
            Fan-->>Main: isFanStarted = false
        else coolantInject ≤ 68.0°C
            Main->>Fan: startFan()
            Fan-->>Main: isFanStarted = true
        end
        
        alt coolantInject ≤ 65.0°C & heatedAtLeastOnce
            Main->>Fan: stopFan()
            Main->>DefrostValve: startDefrost()
            DefrostValve-->>Main: isDefrostStarted = true
            Main->>Main: mode = "defrost"
        end
    end
    
    alt isDefrostStarted & coolantIntake ≥ 5.0°C
        Main->>DefrostValve: stopDefrost()
        DefrostValve-->>Main: isDefrostStarted = false
        Main->>Fan: startFan()
        Fan-->>Main: isFanStarted = true
        Main->>Main: heatedAtLeastOnce = false
        Main->>Main: mode = "work"
    end

    Main->>Display: reDrawScreen()
    Note over Display: Update display with temps and states
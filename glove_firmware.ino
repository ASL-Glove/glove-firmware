#include <Arduino_BMI270_BMM150.h>

#define ANALOG_INPUT_NUM 5
#define DIGITAL_INPUT_NUM 4

// Hardware Pin Definitions (Nano 33 BLE Sense Rev2)
// Reserved: A4 & A5 for I2C bus
const int ANALOG_PINS[5]  = {A0, A1, A2, A3, A6};
const int DIGITAL_PINS[4] = {2, 3, 4, 5};

// Structure to hold normalized sensor readings
struct SensorFrame {
    float analogNormalized[5];
    bool digitalState[4];
    float gyroNormalized[3]; // X, Y, Z angular velocity [-1.0, 1.0]
};

void setup() {
    Serial.begin(115200);

    // Wait for Serial monitor connection (optional, useful for debugging)
    while (!Serial && millis() < 4000);

    // Pin Configuration
    for (int i = 0; i < 5; i++) {
        pinMode(ANALOG_PINS[i], INPUT);
    }
    for (int i = 0; i < 4; i++) {
        pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
    }

    // Initialize Rev2 IMU
    if (!IMU.begin()) {
        Serial.println("Error: Failed to initialize BMI270 IMU!");
        while (1);
    }

    // --- BLE Init ---
    if (!BLE.begin()) {
        Serial.println("Error: Failed to initialize BLE!");
        while (1);
    }
    BLE.setLocalName("ASL Glove");
    BLE.setAdvertisedService(aslService);
    aslService.addCharacteristic(letterChar);
    BLE.addService(aslService);
    letterChar.writeValue(0);  // initial value
    BLE.advertise();

    Serial.println("ASL Glove Initialized. Waiting for BLE connection...");
}

void loop() {
    SensorFrame frame;
    readSensors(frame);

    // Stub classifier (replace with TFLite model inference later)
    char predictedLetter = stubInferenceEngine(frame);

    // Send over BLE if a central is connected and the letter changed
    BLEDevice central = BLE.central();
    if (central && central.connected()) {
        if (letterChar.value() != (uint8_t)predictedLetter) {
            letterChar.writeValue((uint8_t)predictedLetter);
        }
    }

    // Print raw normalized inputs and predicted letter to Serial Monitor
    printFrameAndPrediction(frame, predictedLetter);

    delay(200); // 5 Hz update rate
}

// Read physical inputs and normalize values
void readSensors(SensorFrame &frame) {
    for (int i = 0; i < ANALOG_INPUT_NUM; i++) {
        // 10-bit ADC read (0 - 1023) scaled to 0.0 - 1.0 range
        frame.analogNormalized[i] = analogRead(ANALOG_PINS[i]) / 1023.0f;
    }
    for (int i = 0; i < DIGITAL_INPUT_NUM; i++) {
        // Active LOW contacts pulled to GND when pressed
        frame.digitalState[i] = (digitalRead(DIGITAL_PINS[i]) == LOW);
    }

    // Read Gyroscope (+/- 2000 deg/s scaled to [-1.0, 1.0])
    if (IMU.gyroscopeAvailable()) {
        float gx, gy, gz;
        IMU.readGyroscope(gx, gy, gz);
        frame.gyroNormalized[0] = gx / 2000.0f;
        frame.gyroNormalized[1] = gy / 2000.0f;
        frame.gyroNormalized[2] = gz / 2000.0f;
    }
}

// Placeholder function to simulate classification before TFLite model is added
char stubInferenceEngine(const SensorFrame &frame) {
    // Simple heuristic placeholder
    if (frame.digitalState[0] && frame.digitalState[1]) {
        return 'A';
    } else if (frame.analogNormalized[0] > 0.7f) {
        return 'B';
    } else if (frame.analogNormalized[1] > 0.7f) {
        return 'C';
    }
    return 'D';
}

// Format output for Serial Monitor inspection
void printFrameAndPrediction(const SensorFrame &frame, char letter) {
    Serial.print("Analog: ");
    for (int i = 0; i < ANALOG_INPUT_NUM; i++) {
        Serial.print(frame.analogNormalized[i], 2);
        Serial.print(" ");
    }

    Serial.print("| Digital: ");
    for (int i = 0; i < DIGITAL_INPUT_NUM; i++) {
        Serial.print(frame.digitalState[i]);
        Serial.print(" ");
    }

    Serial.print(" | Gyro(X,Y,Z): ");
    Serial.print(frame.gyroNormalized[0], 2);
    Serial.print(" ");
    Serial.print(frame.gyroNormalized[1], 2);
    Serial.print(" ");
    Serial.print(frame.gyroNormalized[2], 2);

    Serial.print(" --> Letter: ");
    Serial.println(letter);
}

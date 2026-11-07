/**
 * Project: OutBound Cyberdeck
 * Controller: ESP32-S3
 * Description: Main firmware initialization for display, audio, matrix, inputs, and wireless (ESP-NOW).
 */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>

// --- Pin Definitions (Matching your OutBound Schematic) ---
// ILI9341 Display SPI Interface
const int TFT_DC  = 5;
const int TFT_CS  = 3;
const int TFT_RST = 4;

// MAX7219 LED Matrix Pins
const int MATRIX_DIN  = 1;
const int MATRIX_CLK  = 13;
const int MATRIX_LOAD = 12; // CS / LOAD

// MAX98357A I2S Audio Amplifier Pins
const int I2S_DIN   = 7;
const int I2S_BCLK  = 16;
const int I2S_LRCLK = 14;

// User Input Pins (SW2 is on GPIO2 as requested)
const int BUTTON_SW2 = 2; 
const int BUTTON_SW3 = 4; 

// Structure for ESP-NOW peer messaging
typedef struct cyberdeck_message {
    char deviceName[32];
    int commandId;
    float batteryVoltage;
} cyberdeck_message;

cyberdeck_message incomingData;
cyberdeck_message outgoingData;

// ESP-NOW Receive Callback
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingPacket, int len) {
    memcpy(&incomingData, incomingPacket, sizeof(incomingData));
    Serial.print("Received data packet, bytes: ");
    Serial.println(len);
}

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- OutBound Cyberdeck Booting Up ---");

    // Configure Input Buttons with Internal Pull-ups
    pinMode(BUTTON_SW2, INPUT_PULLUP);
    pinMode(BUTTON_SW3, INPUT_PULLUP);

    // Initialize SPI Bus for Display & Matrix peripherals
    SPI.begin();
    Serial.println("SPI bus initialized.");

    // Initialize Wi-Fi in Station Mode for ESP-NOW wireless features
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
    } else {
        Serial.println("ESP-NOW initialized successfully.");
        esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    }

    Serial.println("Setup complete. Entering main loop.");
}

void loop() {
    // Check button on GPIO2 (SW2)
    if (digitalRead(BUTTON_SW2) == LOW) {
        Serial.println("Action: Button SW2 on GPIO2 pressed!");
        
        // Example: Prepare an ESP-NOW broadcast message
        strcpy(outgoingData.deviceName, "OutBound-S3");
        outgoingData.commandId = 101;
        outgoingData.batteryVoltage = 4.12; // Placeholder for battery telemetry
        
        // Add your display update or audio trigger logic here
        
        delay(300); // Simple debounce delay
    }

    // Check button SW3
    if (digitalRead(BUTTON_SW3) == LOW) {
        Serial.println("Action: Button SW3 pressed!");
        delay(300);
    }

    // Background tasks
    delay(50);
}

#include <Arduino.h>
#include "config.h"

// Boot mode detection
bool configModeRequested = false;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    delay(1000);
    
    debugLog("ESP8266 RFM69 Gateway v2 Starting...");
    debugLog("Compiled: " + String(__DATE__) + " " + String(__TIME__));
    
    // Check configuration GPIO pin
    configModeRequested = checkConfigurationMode();
    
    if (configModeRequested) {
        debugLog("Configuration mode requested via GPIO");
        enterConfigurationMode();
    } else {
        debugLog("Starting normal gateway operation");
        enterNormalMode();
    }
}

void loop() {
    // This should never be reached as both modes have their own loops
    // But just in case, restart the device
    debugLog("Unexpected return to main loop - restarting");
    delay(1000);
    ESP.restart();
}

bool checkConfigurationMode() {
    debugLog("Checking configuration GPIO pin " + String(CONF_GPIO_NUM));
    
    // Configure the pin as input with pullup
    pinMode(CONF_GPIO_NUM, INPUT_PULLUP);
    
    // Check if the pin is in the configured hold state
    bool pinActive = (digitalRead(CONF_GPIO_NUM) == CONF_GPIO_HOLD_STATE);
    
    if (!pinActive) {
        debugLog("Configuration GPIO not active");
        return false;
    }
    
    debugLog("Configuration GPIO active, checking hold time...");
    
    // Check if pin is held in the active state for the required duration
    unsigned long startTime = millis();
    unsigned long holdTime = 0;
    
    while (holdTime < CONF_GPIO_HOLD_MS) {
        if (digitalRead(CONF_GPIO_NUM) != CONF_GPIO_HOLD_STATE) {
            debugLog("Configuration GPIO released before timeout");
            return false;
        }
        
        holdTime = millis() - startTime;
        delay(50); // Small delay to prevent excessive polling
        
        // Provide feedback every second
        if (holdTime % 1000 == 0 && holdTime > 0) {
            debugLog("Hold time: " + String(holdTime) + "ms / " + String(CONF_GPIO_HOLD_MS) + "ms");
        }
    }
    
    debugLog("Configuration mode activated - GPIO held for " + String(holdTime) + "ms");
    return true;
}

// Additional utility functions can be added here as needed

void systemInfo() {
    debugLog("=== System Information ===");
    debugLog("Chip ID: " + String(ESP.getChipId()));
    debugLog("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    debugLog("Flash Size: " + String(ESP.getFlashChipSize() / 1024) + " KB");
    debugLog("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    debugLog("SDK Version: " + String(ESP.getSdkVersion()));
    debugLog("Boot Version: " + String(ESP.getBootVersion()));
    debugLog("Boot Mode: " + String(ESP.getBootMode()));
    debugLog("Reset Reason: " + String(ESP.getResetReason()));
    debugLog("Reset Info: " + ESP.getResetInfo());
    debugLog("==========================");
}
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <IPAddress.h>

// Forward declarations
class AsyncWebServerRequest;

// Compile-time constants (if not already from build flags)

// Radio related defines
#ifndef RFM69_FREQUENCY
// #define RFM69_FREQUENCY     RF69_315MHZ             // 31
// #define RFM69_FREQUENCY     RF69_433MHZ             // 43
#define RFM69_FREQUENCY     RF69_868MHZ             // 86
// #define RFM69_FREQUENCY     RF69_915MHZ             // 91
#endif

#ifndef IS_RFM69HW_HCW          // Is it a HCW/HW Radio Chip
#define IS_RFM69HW_HCW      1
#endif

#ifndef IS_RFM69_SPY_MODE       // Should we operate in RFM69 Spy mode (listening to all bands)
#define IS_RFM69_SPY_MODE   0
#endif

#ifndef RFM69_ENABLE_ATC         // Enable Automatic Transmission Control of Radio
//#define RFM69_ENABLE_ATC    1  // Uncomment to enable ATC, 
#endif

// RFM69 Hardware pin definitions (can be overridden at compile time)
#ifndef RFM69_CS_PIN
#define RFM69_CS_PIN 15  // GPIO15 (D8 on WeMos Mini)
#endif

#ifndef RFM69_IRQ_PIN
#define RFM69_IRQ_PIN 4  // GPIO4 (D2 on WeMos Mini)
#endif

#ifndef RFM69_RST_PIN
#define RFM69_RST_PIN 5  // GPIO5 (D1 on WeMos Mini)
#endif

// Derived definitions
#define RFM69_IRQN digitalPinToInterrupt(RFM69_IRQ_PIN)

// General Directives
#ifndef ENABLE_EXPERT_CONFIG
#define ENABLE_EXPERT_CONFIG false
#endif

#ifndef EXPERT_MODE_PASSWORD
#define EXPERT_MODE_PASSWORD "IoT@1234" 
#endif

// Boot time Configuration GPIO Settings
#ifndef CONF_GPIO_NUM
#define CONF_GPIO_NUM 0
#endif

#ifndef CONF_GPIO_HOLD_MS
#define CONF_GPIO_HOLD_MS 5000
#endif

#ifndef CONF_GPIO_HOLD_STATE
#define CONF_GPIO_HOLD_STATE LOW
#endif

// Configuration structure version for EEPROM compatibility
#define CONFIG_VERSION 1
#define CONFIG_MAGIC 0xDEADBEEF

// Maximum string lengths
#define MAX_STRING_LENGTH 32
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define ENCRYPTION_KEY_LENGTH 16

// Structure to hold all configuration variables
struct GatewayConfig {
    // Magic number and version for validation
    uint32_t magic;
    uint8_t version;
    
    // Radio related configuration
    uint8_t networkId;              // 1-255
    uint8_t nodeId;                 // 1-255 (expert mode only)
    char encryptionKey[ENCRYPTION_KEY_LENGTH + 1];  // 16 char string + null terminator
    uint16_t radioPower;            // unsigned short (expert mode only)
    
    // Network related configuration
    bool dhcp;                      // DHCP enable/disable
    IPAddress staticIP;             // Static IP address
    IPAddress netmask;              // Network mask
    IPAddress gateway;              // Gateway address
    IPAddress dns1;                 // Primary DNS
    IPAddress dns2;                 // Secondary DNS
    char wifiSSID[MAX_SSID_LENGTH + 1];
    char wifiPassword[MAX_PASSWORD_LENGTH + 1];
    
    // MQTT related configuration (expert mode only)
    char mqttServer[MAX_STRING_LENGTH + 1];
    uint16_t mqttPort;
    char mqttUser[MAX_STRING_LENGTH + 1];
    char mqttPass[MAX_PASSWORD_LENGTH + 1];
    
    // Access Point related configuration
    char apName[MAX_SSID_LENGTH + 1];      // expert mode only
    char apUser[MAX_STRING_LENGTH + 1];
    char apPassword[MAX_PASSWORD_LENGTH + 1];
    
    // System configuration
    bool expertMode;                // Expert mode enable/disable
    
    // Checksum for data integrity
    uint32_t checksum;
};

// Default configuration values
extern const GatewayConfig defaultConfig;

// EEPROM management functions
bool saveConfig(const GatewayConfig& config);
bool loadConfig(GatewayConfig& config);
bool factoryReset();
uint32_t calculateChecksum(const GatewayConfig& config);
bool validateConfig(const GatewayConfig& config);

// Configuration mode functions
void enterConfigurationMode();
void startCaptivePortal();
void handleWebRequests();
void setupWebServer();
void handleHomePage(AsyncWebServerRequest *request);
void handleRadioPage(AsyncWebServerRequest *request);
void handleRadioSave(AsyncWebServerRequest *request);
void handleNetworkPage(AsyncWebServerRequest *request);
void handleNetworkSave(AsyncWebServerRequest *request);
void handleMqttPage(AsyncWebServerRequest *request);
void handleMqttSave(AsyncWebServerRequest *request);
void handleApPage(AsyncWebServerRequest *request);
void handleApSave(AsyncWebServerRequest *request);
void handleSystemPage(AsyncWebServerRequest *request);
void handleSystemAction(AsyncWebServerRequest *request);
void handleApiStatus(AsyncWebServerRequest *request);
void handleApiReboot(AsyncWebServerRequest *request);
void handleApiFactoryReset(AsyncWebServerRequest *request);

// Normal mode functions  
void enterNormalMode();
bool initializeWiFi();
bool initializeMQTT();
bool initializeRadio();
void setupMqttTopics();
void handleNormalModeLoop();
bool connectMqtt();
void publishStatus();
void handleRadioMessages();
void processRadioToMqtt(uint8_t senderId, uint8_t targetId, const String& message, int16_t rssi);
void onMqttMessage(char* topic, byte* payload, unsigned int length);
void handleMqttCommand(const String& topic, const String& message);
void handleRadioSendCommand(const String& message);

// Main application functions
bool checkConfigurationMode();

// Utility functions
void printConfig(const GatewayConfig& config);
void debugLog(const String& message);

#endif // CONFIG_H
#include "config.h"
#include <EEPROM.h>

// Default configuration values
const GatewayConfig defaultConfig = {
    CONFIG_MAGIC,                   // magic
    CONFIG_VERSION,                 // version
    
    // Access Point configuration defaults
    DEF_CFG_AP_NAME,                // apName (expert mode)
    DEF_CFG_AP_USER,                // apUser
    DEF_CFG_AP_PASS,                // apPassword
    
    // Radio configuration defaults
    DEF_CFG_NETWORK_ID,             // networkId
    DEF_CFG_NODE_ID,                // nodeId (Gateway is always 1)
    DEF_CFG_ENCRYPTON_KEY,          // encryptionKey (16 characters)
    DEF_CFG_RADIO_POWER,            // radioPower (Max power, expert mode, 18=14dB, 30=20dB)
    
    // Network configuration defaults
    DEF_CFG_DHCP,                   // dhcp
    IPAddress(DEF_CFG_STATC_IP),    // staticIP
    IPAddress(DEF_CFG_NETMASK),     // netmask
    IPAddress(DEF_CFG_GATEWAY),     // gateway
    IPAddress(DEF_CFG_DNS1),        // dns1
    IPAddress(DEF_CFG_DNS2),        // dns2
    DEF_CFG_WFI_SSID,               // wifiSSID
    DEF_CFG_WIFI_PASS,              // wifiPassword
    
    // MQTT configuration defaults (expert mode)
    DEF_CFG_MQTT_SERVER,            // mqttServer
    DEF_CFG_MQTT_PORT,              // mqttPort
    DEF_CFG_MQTT_USER,              // mqttUser
    DEF_CFG_MQTT_PASS,              // mqttPass
    DEF_CFG_MQTT_TOPIC_PREFIX_IN,   // mqttTopicPrexixIn
    DEF_CFG_MQTT_TOPIC_PREFIX_OUT,  // mqttTopicPrefixOut
    
    // System configuration defaults
    DEF_CFG_ENABLE_EXPPERT_CONF,    // expertMode
    
    0                              // checksum (will be calculated)
};

uint32_t calculateChecksum(const GatewayConfig& config) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)&config;
    size_t size = sizeof(GatewayConfig) - sizeof(config.checksum);  // Exclude checksum field
    
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
        checksum = (checksum << 1) | (checksum >> 31);  // Rotate left
    }
    
    return checksum;
}

bool validateConfig(const GatewayConfig& config) {
    // Check magic number and version
    if (config.magic != CONFIG_MAGIC || config.version != CONFIG_VERSION) {
        debugLog("Config validation failed: Invalid magic or version");
        return false;
    }
    
    // Validate checksum
    uint32_t expectedChecksum = calculateChecksum(config);
    if (config.checksum != expectedChecksum) {
        debugLog("Config validation failed: Checksum mismatch");
        return false;
    }
    
    // Validate range values
    if (config.networkId == 0 || config.networkId > 255) {
        debugLog("Config validation failed: Invalid network ID");
        return false;
    }
    
    if (config.nodeId == 0 || config.nodeId > 255) {
        debugLog("Config validation failed: Invalid node ID");
        return false;
    }
    
    if (config.mqttPort == 0 || config.mqttPort > 65535) {
        debugLog("Config validation failed: Invalid MQTT port");
        return false;
    }
    
    return true;
}

bool saveConfig(const GatewayConfig& config) {
    debugLog("Saving configuration to EEPROM...");
    
    // Create a copy and calculate checksum
    GatewayConfig configCopy = config;
    configCopy.checksum = calculateChecksum(configCopy);
    
    // Initialize EEPROM
    EEPROM.begin(sizeof(GatewayConfig));
    
    // Write configuration to EEPROM
    uint8_t* data = (uint8_t*)&configCopy;
    for (size_t i = 0; i < sizeof(GatewayConfig); i++) {
        EEPROM.write(i, data[i]);
    }
    
    // Commit changes
    bool success = EEPROM.commit();
    EEPROM.end();
    
    if (success) {
        debugLog("Configuration saved successfully");
    } else {
        debugLog("Failed to save configuration");
    }
    
    return success;
}

bool loadConfig(GatewayConfig& config) {
    debugLog("Loading configuration from EEPROM...");
    
    // Initialize EEPROM
    EEPROM.begin(sizeof(GatewayConfig));
    
    // Read configuration from EEPROM
    uint8_t* data = (uint8_t*)&config;
    for (size_t i = 0; i < sizeof(GatewayConfig); i++) {
        data[i] = EEPROM.read(i);
    }
    
    EEPROM.end();
    
    // Validate loaded configuration
    bool valid = validateConfig(config);
    
    if (valid) {
        debugLog("Configuration loaded and validated successfully");
    } else {
        debugLog("Configuration validation failed, using defaults");
    }
    
    return valid;
}

bool factoryReset() {
    debugLog("Performing factory reset...");
    
    // Create default configuration with calculated checksum
    GatewayConfig resetConfig = defaultConfig;
    resetConfig.checksum = calculateChecksum(resetConfig);
    
    // Save default configuration
    if (saveConfig(resetConfig)) {
        debugLog("Factory reset completed successfully");
        return true ;
    } else {
        debugLog("Factory reset failed");
        return false ;
    }
}

void printConfig(const GatewayConfig& config) {
    Serial.println("=== Gateway Configuration ===");
    Serial.printf("Magic: 0x%08X, Version: %d\n", config.magic, config.version);
    Serial.printf("Network ID: %d, Node ID: %d\n", config.networkId, config.nodeId);
    Serial.printf("Encryption Key: %s\n", config.encryptionKey);
    Serial.printf("Radio Power: %d\n", config.radioPower);
    Serial.printf("DHCP: %s\n", config.dhcp ? "enabled" : "disabled");
    
    if (!config.dhcp) {
        Serial.printf("Static IP: %s\n", config.staticIP.toString().c_str());
        Serial.printf("Netmask: %s\n", config.netmask.toString().c_str());
        Serial.printf("Gateway: %s\n", config.gateway.toString().c_str());
        Serial.printf("DNS1: %s\n", config.dns1.toString().c_str());
        Serial.printf("DNS2: %s\n", config.dns2.toString().c_str());
    }
    
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID);
    Serial.printf("MQTT Server: %s:%d\n", config.mqttServer, config.mqttPort);
    Serial.printf("MQTT User: %s\n", config.mqttUser);
    Serial.printf("MQTT Topic Prefix In: %s\n", config.mqttTopicPrefixIn);
    Serial.printf("MQTT Topic Prefix Out: %s\n", config.mqttTopicPrefixOut);
    Serial.printf("AP Name: %s\n", config.apName);
    Serial.printf("AP User: %s\n", config.apUser);
    Serial.printf("Expert Mode: %s\n", config.expertMode ? "enabled" : "disabled");
    Serial.printf("Checksum: 0x%08X\n", config.checksum);
    Serial.println("=============================");
}

void debugLog(const String& message) {
    Serial.print("[DEBUG] ");
    Serial.println(message);
}
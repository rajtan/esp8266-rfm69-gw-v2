#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <ArduinoJson.h>

// Global objects for normal mode operation
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#ifdef RFM69_ENABLE_ATC
    RFM69_ATC radio;
#else 
    RFM69 radio;
#endif

GatewayConfig activeConfig;

// Connection status
bool wifiConnected = false;
bool mqttConnected = false;
bool radioInitialized = false;

// Timing variables
unsigned long lastMqttReconnect = 0;
unsigned long lastRadioCheck = 0;
unsigned long lastStatusReport = 0;

const unsigned long MQTT_RECONNECT_INTERVAL = 5000;  // 5 seconds
const unsigned long RADIO_CHECK_INTERVAL = 100;     // 100ms
const unsigned long STATUS_REPORT_INTERVAL = 30000; // 30 seconds

// MQTT topics
String mqttBaseTopic;
String mqttStatusTopic;
String mqttCommandTopic;
String mqttRadioTopic;

void enterNormalMode() {
    debugLog("Entering normal mode");
    
    // Load configuration from EEPROM
    if (!loadConfig(activeConfig)) {
        debugLog("Failed to load configuration, performing factory reset");
        factoryReset();
        ESP.restart();
        return;
    }
    
    printConfig(activeConfig);
    
    // Initialize components
    if (!initializeWiFi()) {
        debugLog("WiFi initialization failed, entering configuration mode");
        enterConfigurationMode();
        return;
    }
    
    if (!initializeRadio()) {
        debugLog("Radio initialization failed, continuing without radio");
    }
    
    if (!initializeMQTT()) {
        debugLog("MQTT initialization failed, continuing without MQTT");
    }
    
    setupMqttTopics();
    
    debugLog("Normal mode initialization completed");
    
    // Main operation loop
    while (true) {
        handleNormalModeLoop();
        delay(10);
    }
}

bool initializeWiFi() {
    debugLog("Initializing WiFi connection...");
    
    WiFi.mode(WIFI_STA);
    
    if (!activeConfig.dhcp) {
        debugLog("Using static IP configuration");
        WiFi.config(activeConfig.staticIP, activeConfig.gateway, activeConfig.netmask, activeConfig.dns1, activeConfig.dns2);
    }
    
    WiFi.begin(activeConfig.wifiSSID, activeConfig.wifiPassword);
    
    int attempts = 0;
    const int maxAttempts = 20; // 10 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        debugLog("WiFi connected successfully");
        debugLog("IP address: " + WiFi.localIP().toString());
        debugLog("Gateway: " + WiFi.gatewayIP().toString());
        debugLog("DNS: " + WiFi.dnsIP().toString());
        return true;
    } else {
        debugLog("WiFi connection failed");
        wifiConnected = false;
        return false;
    }
}

bool initializeMQTT() {
    if (!wifiConnected) {
        debugLog("Cannot initialize MQTT: WiFi not connected");
        return false;
    }
    
    debugLog("Initializing MQTT connection...");
    
    mqttClient.setServer(activeConfig.mqttServer, activeConfig.mqttPort);
    mqttClient.setCallback(onMqttMessage);
    
    return connectMqtt();
}

bool connectMqtt() {
    if (!wifiConnected) {
        return false;
    }
    
    String clientId = "ESP8266Gateway-" + String(activeConfig.nodeId);
    
    bool connected = false;
    if (strlen(activeConfig.mqttUser) > 0) {
        connected = mqttClient.connect(clientId.c_str(), activeConfig.mqttUser, activeConfig.mqttPass);
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        mqttConnected = true;
        debugLog("MQTT connected successfully");
        
        // Subscribe to command topic
        String commandTopic = mqttCommandTopic + "/+";
        mqttClient.subscribe(commandTopic.c_str());
        debugLog("Subscribed to: " + commandTopic);
        
        // Publish status
        publishStatus();
        
        return true;
    } else {
        mqttConnected = false;
        debugLog("MQTT connection failed, error: " + String(mqttClient.state()));
        return false;
    }
}

bool initializeRadio() {
    debugLog("Initializing RFM69 radio...");
    
    // Initialize radio with pins from config.h
    if (!radio.initialize(RFM69_FREQUENCY, activeConfig.nodeId, activeConfig.networkId)) {
        debugLog("Radio initialization failed");
        return false;
    }
#ifdef IS_RFM69HW_HCW   
    // Set high power mode if using RFM69HCW
    radio.setHighPower();
#endif
    // Set power level (0-31)
    radio.setPowerLevel(activeConfig.radioPower);

#ifdef IS_RFM69_SPY_MODE   
    // Set high power mode if using RFM69HCW
    radio.spyMode(IS_RFM69_SPY_MODE);
#endif

    // Set encryption if key is provided
    if (strlen(activeConfig.encryptionKey) > 0) {
        radio.encrypt(activeConfig.encryptionKey);
        debugLog("Radio encryption enabled");
    }
    
    radioInitialized = true;
    debugLog("Radio initialized successfully");
    debugLog("Frequency: " + String(RFM69_FREQUENCY) + " MHz");
    debugLog("Network ID: " + String(activeConfig.networkId));
    debugLog("Node ID: " + String(activeConfig.nodeId));
    debugLog("Power Level: " + String(activeConfig.radioPower));
    debugLog("Pin Configuration - CS: " + String(RFM69_CS_PIN) + ", IRQ: " + String(RFM69_IRQ_PIN) + ", RST: " + String(RFM69_RST_PIN));
    
    return true;
}

void setupMqttTopics() {
    // Use configured topic prefixes instead of hardcoded ones
    String inPrefix = String(activeConfig.mqttTopicPrefixIn);
    String outPrefix = String(activeConfig.mqttTopicPrefixOut);
    
    // Ensure prefixes end with '/' if not empty
    if (inPrefix.length() > 0 && !inPrefix.endsWith("/")) {
        inPrefix += "/";
    }
    if (outPrefix.length() > 0 && !outPrefix.endsWith("/")) {
        outPrefix += "/";
    }
    
    mqttBaseTopic = outPrefix + String(activeConfig.nodeId);
    mqttStatusTopic = mqttBaseTopic + "/status";
    mqttCommandTopic = inPrefix + String(activeConfig.nodeId) + "/command";
    mqttRadioTopic = mqttBaseTopic + "/radio";
    
    debugLog("MQTT Topic Configuration:");
    debugLog("  Incoming Prefix: " + inPrefix);
    debugLog("  Outgoing Prefix: " + outPrefix);
    debugLog("  Base Topic: " + mqttBaseTopic);
    debugLog("  Command Topic: " + mqttCommandTopic);
}

void handleNormalModeLoop() {
    // Handle WiFi connection
    if (!WiFi.isConnected()) {
        if (wifiConnected) {
            debugLog("WiFi connection lost, attempting reconnect");
            wifiConnected = false;
            mqttConnected = false;
        }
        
        // Attempt WiFi reconnection
        if (millis() - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
            initializeWiFi();
            lastMqttReconnect = millis();
        }
    } else {
        if (!wifiConnected) {
            wifiConnected = true;
            debugLog("WiFi reconnected");
        }
    }
    
    // Handle MQTT connection
    if (wifiConnected && !mqttClient.connected()) {
        if (mqttConnected) {
            debugLog("MQTT connection lost");
            mqttConnected = false;
        }
        
        if (millis() - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
            connectMqtt();
            lastMqttReconnect = millis();
        }
    }
    
    // Process MQTT messages
    if (mqttConnected) {
        mqttClient.loop();
    }
    
    // Handle radio communication
    if (radioInitialized && millis() - lastRadioCheck > RADIO_CHECK_INTERVAL) {
        handleRadioMessages();
        lastRadioCheck = millis();
    }
    
    // Publish periodic status
    if (mqttConnected && millis() - lastStatusReport > STATUS_REPORT_INTERVAL) {
        publishStatus();
        lastStatusReport = millis();
    }
}

void handleRadioMessages() {
    if (radio.receiveDone()) {
        // Get sender information
        uint8_t senderId = radio.SENDERID;
        uint8_t targetId = radio.TARGETID;
        int16_t rssi = radio.RSSI;
        bool ackRequested = radio.ACKRequested();
        
        // Get message data
        String messageData = "";
        for (uint8_t i = 0; i < radio.DATALEN; i++) {
            messageData += (char)radio.DATA[i];
        }
        
        debugLog("Radio message received from node " + String(senderId) + ": " + messageData);
        debugLog("RSSI: " + String(rssi) + " dBm");
        
        // Send ACK if requested
        if (ackRequested) {
            radio.sendACK();
            debugLog("ACK sent to node " + String(senderId));
        }
        
        // Process and forward to MQTT
        processRadioToMqtt(senderId, targetId, messageData, rssi);
    }
}

void processRadioToMqtt(uint8_t senderId, uint8_t targetId, const String& message, int16_t rssi) {
    if (!mqttConnected) {
        debugLog("Cannot forward to MQTT: not connected");
        return;
    }
    
    // Create JSON message for MQTT
    DynamicJsonDocument doc(512);
    doc["timestamp"] = millis();
    doc["senderId"] = senderId;
    doc["targetId"] = targetId;
    doc["rssi"] = rssi;
    doc["message"] = message;
    
    // Try to parse the radio message as JSON for structured data
    DynamicJsonDocument radioDoc(256);
    if (deserializeJson(radioDoc, message) == DeserializationError::Ok) {
        doc["data"] = radioDoc;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Use configured outgoing prefix for radio messages
    String outPrefix = String(activeConfig.mqttTopicPrefixOut);
    if (outPrefix.length() > 0 && !outPrefix.endsWith("/")) {
        outPrefix += "/";
    }
    
    String topic = outPrefix + String(activeConfig.nodeId) + "/radio/received/" + String(senderId);
    if (mqttClient.publish(topic.c_str(), jsonString.c_str())) {
        debugLog("Forwarded to MQTT topic: " + topic);
    } else {
        debugLog("Failed to publish to MQTT");
    }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String message = "";
    
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    debugLog("MQTT message received on topic: " + topicStr);
    debugLog("Message: " + message);
    
    // Parse command topic
    if (topicStr.startsWith(mqttCommandTopic)) {
        handleMqttCommand(topicStr, message);
    }
}

void handleMqttCommand(const String& topic, const String& message) {
    // Extract command from topic: gateway/1/command/{command}
    int lastSlash = topic.lastIndexOf('/');
    if (lastSlash == -1) return;
    
    String command = topic.substring(lastSlash + 1);
    
    debugLog("Processing command: " + command);
    
    if (command == "send") {
        handleRadioSendCommand(message);
    } else if (command == "status") {
        publishStatus();
    } else if (command == "reboot") {
        debugLog("Reboot command received via MQTT");
        ESP.restart();
    } else {
        debugLog("Unknown command: " + command);
    }
}

void handleRadioSendCommand(const String& message) {
    if (!radioInitialized) {
        debugLog("Cannot send radio message: radio not initialized");
        return;
    }
    
    // Parse JSON command
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, message) != DeserializationError::Ok) {
        debugLog("Invalid JSON in send command");
        return;
    }
    
    if (!doc.containsKey("nodeId") || !doc.containsKey("message")) {
        debugLog("Send command missing required fields (nodeId, message)");
        return;
    }
    
    uint8_t targetNode = doc["nodeId"];
    String payload = doc["message"];
    bool requestAck = doc["ack"] | false;
    
    debugLog("Sending radio message to node " + String(targetNode) + ": " + payload);
    
    bool success = false;
    if (requestAck) {
        success = radio.sendWithRetry(targetNode, payload.c_str(), payload.length(), 3, 100);
    } else {
        radio.send(targetNode, payload.c_str(), payload.length());
        success = true; // No way to verify without ACK
    }
    
    // Report result back to MQTT
    DynamicJsonDocument response(256);
    response["command"] = "send";
    response["targetNode"] = targetNode;
    response["success"] = success;
    response["timestamp"] = millis();
    
    String responseString;
    serializeJson(response, responseString);
    
    String responseTopic = mqttBaseTopic + "/response/send";
    mqttClient.publish(responseTopic.c_str(), responseString.c_str());
    
    debugLog("Radio send result: " + String(success ? "success" : "failed"));
}

void publishStatus() {
    if (!mqttConnected) return;
    
    DynamicJsonDocument doc(512);
    doc["timestamp"] = millis();
    doc["uptime"] = millis();
    doc["nodeId"] = activeConfig.nodeId;
    doc["networkId"] = activeConfig.networkId;
    doc["wifiConnected"] = wifiConnected;
    doc["mqttConnected"] = mqttConnected;
    doc["radioInitialized"] = radioInitialized;
    
    if (wifiConnected) {
        doc["wifiSSID"] = WiFi.SSID();
        doc["wifiIP"] = WiFi.localIP().toString();
        doc["wifiRSSI"] = WiFi.RSSI();
    }
    
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    
    String statusString;
    serializeJson(doc, statusString);
    
    if (mqttClient.publish(mqttStatusTopic.c_str(), statusString.c_str(), true)) {
        debugLog("Status published to MQTT");
    }
}
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

// Global variables for configuration mode
AsyncWebServer webServer(80);
DNSServer dnsServer;
GatewayConfig currentConfig;
bool configModeActive = false;

// DNS and captive portal settings
const byte DNS_PORT = 53;
const char* CAPTIVE_PORTAL_DOMAIN = "gateway.local";

// HTML templates
const char* HTML_HEADER PROGMEM = R"**(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 RFM69 Gateway</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; font-size: 140%; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 10px; border-radius: 6px; }
        .header { text-align: center; color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }
        .nav { margin: 20px 0; position: relative; }
        .nav ul { list-style: none; margin: 0; padding: 0; display: flex; justify-content: center; flex-wrap: wrap; }
        .nav li { margin: 5px; }
        .nav a { display: block; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; transition: background-color 0.3s; }
        .nav a:hover { background: #0056b3; }
        
        /* Hamburger menu button (hidden by default) */
        .nav-toggle { display: none; background: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; position: absolute; top: 0; right: 0; }
        .nav-toggle:hover { background: #0056b3; }
        
        /* Mobile responsive styles */
        @media (max-width: 768px) {
            .nav ul { 
                display: none; 
                flex-direction: column; 
                position: absolute; 
                top: 50px; 
                left: 0; 
                right: 0; 
                background: white; 
                box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
                border-radius: 5px; 
                z-index: 1000;
            }
            .nav ul.active { display: flex; }
            .nav li { margin: 0; }
            .nav a { margin: 0; border-radius: 0; border-bottom: 1px solid #eee; }
            .nav a:last-child { border-bottom: none; }
            .nav-toggle { display: block; }
        }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; font-size: 100%;}
        .btn { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 5px; cursor: pointer; margin: 10px 5px; }
        .btn:hover { background: #218838; }
        .btn-danger { background: #dc3545; }
        .btn-danger:hover { background: #c82333; }
        .btn-warning { background: #ffc107; color: #212529; }
        .btn-warning:hover { background: #e0a800; }
        .expert-only { background: #fff3cd; padding: 10px; border-left: 4px solid #ffc107; margin: 10px 0; }
        .success { color: green; margin: 10px 0; }
        .error { color: red; margin: 10px 0; }
    </style>
</head>
<script>
function toggleNav() {
    var navMenu = document.getElementById('nav-menu');
    navMenu.classList.toggle('active');
}

// Close menu when clicking outside
document.addEventListener('click', function(event) {
    var nav = document.querySelector('.nav');
    var navMenu = document.getElementById('nav-menu');
    var navToggle = document.querySelector('.nav-toggle');
    
    if (!nav.contains(event.target)) {
        navMenu.classList.remove('active');
    }
});

// Close menu when window is resized to desktop size
window.addEventListener('resize', function() {
    var navMenu = document.getElementById('nav-menu');
    if (window.innerWidth > 768) {
        navMenu.classList.remove('active');
    }
});
</script>
<body>
<div class="container">
    <h1 class="header">ESP8266 RFM69 Gateway Configuration</h1>
    <div class="nav">
        <button class="nav-toggle" onclick="toggleNav()">☰</button>
        <ul id="nav-menu">
            <li><a href="/">Home</a></li>
            <li><a href="/radio">Radio Config</a></li>
            <li><a href="/network">Network Config</a></li>
            <li><a href="/mqtt">MQTT Config</a></li>
            <li><a href="/ap">Access Point</a></li>
            <li><a href="/system">System</a></li>
        </ul>
    </div>
)**";

const char* HTML_FOOTER = R"(
</div>
<footer>
  <p><center>Copyright 2025 MPS Digital Labs <a href="https://mps.in">https://mps.in</a></center></p>
</footer>
</body>
</html>
)";

void startCaptivePortal() {
    debugLog("Starting captive portal...");
    
    // Start WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(currentConfig.apName, currentConfig.apPassword);
    
    debugLog("Access Point started: " + String(currentConfig.apName));
    debugLog("IP address: " + WiFi.softAPIP().toString());
    
    // Start DNS server for captive portal
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    
    setupWebServer();
    webServer.begin();
    
    configModeActive = true;
    debugLog("Captive portal is now active");
}

void setupWebServer() {
    // Captive portal redirect
    webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://" + WiFi.softAPIP().toString());
    });
    
    // Home page
    webServer.on("/", HTTP_GET, handleHomePage);
    
    // Configuration pages
    webServer.on("/radio", HTTP_GET, handleRadioPage);
    webServer.on("/radio", HTTP_POST, handleRadioSave);
    
    webServer.on("/network", HTTP_GET, handleNetworkPage);
    webServer.on("/network", HTTP_POST, handleNetworkSave);
    
    webServer.on("/mqtt", HTTP_GET, handleMqttPage);
    webServer.on("/mqtt", HTTP_POST, handleMqttSave);
    
    webServer.on("/ap", HTTP_GET, handleApPage);
    webServer.on("/ap", HTTP_POST, handleApSave);
    
    webServer.on("/system", HTTP_GET, handleSystemPage);
    webServer.on("/system", HTTP_POST, handleSystemAction);
    
    // API endpoints
    webServer.on("/api/status", HTTP_GET, handleApiStatus);
    webServer.on("/api/reboot", HTTP_POST, handleApiReboot);
    webServer.on("/api/factory-reset", HTTP_POST, handleApiFactoryReset);
}

void handleHomePage(AsyncWebServerRequest *request) {
    String html = FPSTR(HTML_HEADER);
    html += R"(
    <h2>Gateway Status</h2>
    <div class="form-group">
        <p><strong>Network ID:</strong> )" + String(currentConfig.networkId) + R"(</p>
        <p><strong>Node ID:</strong> )" + String(currentConfig.nodeId) + R"(</p>
        <p><strong>WiFi SSID:</strong> )" + String(currentConfig.wifiSSID) + R"(</p>
        <p><strong>DHCP:</strong> )" + String(currentConfig.dhcp ? "Enabled" : "Disabled") + R"(</p>
        <p><strong>Expert Mode:</strong> )" + String(currentConfig.expertMode ? "Enabled" : "Disabled") + R"(</p>
    </div>
    <h3>Quick Actions</h3>
    <button class="btn" onclick="location.href='/radio'">Configure Radio</button>
    <button class="btn" onclick="location.href='/network'">Configure Network</button>
    <button class="btn btn-warning" onclick="location.href='/system'">System Settings</button>
    )";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleRadioPage(AsyncWebServerRequest *request) {
    String html = FPSTR(HTML_HEADER);
    html += R"(
    <h2>Radio Configuration</h2>
    <form method="POST" action="/radio">
        <div class="form-group">
            <label>Network ID (1-255):</label>
            <input type="number" name="networkId" min="1" max="255" value=")" + String(currentConfig.networkId) + R"(" required>
        </div>
    )";
    
    if (currentConfig.expertMode) {
        html += R"(
        <div class="expert-only">
            <p><strong>Expert Mode Settings:</strong></p>
            <div class="form-group">
                <label>Node ID (1-255):</label>
                <input type="number" name="nodeId" min="1" max="255" value=")" + String(currentConfig.nodeId) + R"(">
            </div>
            <div class="form-group">
                <label>Radio Power (0-31):</label>
                <input type="number" name="radioPower" min="0" max="31" value=")" + String(currentConfig.radioPower) + R"(">
            </div>
        </div>
        )";
    }
    
    html += R"(
        <div class="form-group">
            <label>Encryption Key (16 characters):</label>
            <input type="text" name="encryptionKey" maxlength="16" value=")" + String(currentConfig.encryptionKey) + R"(" required>
        </div>
        <button type="submit" class="btn">Save Radio Configuration</button>
    </form>
    )";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleRadioSave(AsyncWebServerRequest *request) {
    String message = "";
    
    if (request->hasParam("networkId", true)) {
        currentConfig.networkId = request->getParam("networkId", true)->value().toInt();
    }
    
    if (request->hasParam("encryptionKey", true)) {
        String key = request->getParam("encryptionKey", true)->value();
        if (key.length() == 16) {
            strncpy(currentConfig.encryptionKey, key.c_str(), ENCRYPTION_KEY_LENGTH);
            currentConfig.encryptionKey[ENCRYPTION_KEY_LENGTH] = '\0';
        } else {
            message = "Error: Encryption key must be exactly 16 characters";
        }
    }
    
    if (currentConfig.expertMode) {
        if (request->hasParam("nodeId", true)) {
            currentConfig.nodeId = request->getParam("nodeId", true)->value().toInt();
        }
        if (request->hasParam("radioPower", true)) {
            currentConfig.radioPower = request->getParam("radioPower", true)->value().toInt();
        }
    }
    
    if (message.isEmpty()) {
        if (saveConfig(currentConfig)) {
            message = "Radio configuration saved successfully!";
        } else {
            message = "Error saving configuration";
        }
    }
    
    String html = FPSTR(HTML_HEADER);
    html += "<h2>Radio Configuration</h2>";
    html += "<div class='" + String(message.startsWith("Error") ? "error" : "success") + "'>" + message + "</div>";
    html += "<button class='btn' onclick='location.href=\"/radio\"'>Back to Radio Config</button>";
    html += "<button class='btn' onclick='location.href=\"/\"'>Home</button>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleNetworkPage(AsyncWebServerRequest *request) {
    String html = FPSTR(HTML_HEADER);
    html += R"(
    <h2>Network Configuration</h2>
    <form method="POST" action="/network">
        <div class="form-group">
            <label>WiFi SSID:</label>
            <input type="text" name="wifiSSID" maxlength="32" value=")" + String(currentConfig.wifiSSID) + R"(" required>
        </div>
        <div class="form-group">
            <label>WiFi Password:</label>
            <input type="password" name="wifiPassword" maxlength="64" value=")" + String(currentConfig.wifiPassword) + R"(">
        </div>
        <div class="form-group">
            <input type="checkbox" name="dhcp" )" + String(currentConfig.dhcp ? "checked" : "") + R"(> Use DHCP
        </div>
        <div class="form-group">
            <label>Static IP Address:</label>
            <input type="text" name="staticIP" value=")" + currentConfig.staticIP.toString() + R"(" placeholder="192.168.1.100">
        </div>
        <div class="form-group">
            <label>Network Mask:</label>
            <input type="text" name="netmask" value=")" + currentConfig.netmask.toString() + R"(" placeholder="255.255.255.0">
        </div>
        <div class="form-group">
            <label>Gateway:</label>
            <input type="text" name="gateway" value=")" + currentConfig.gateway.toString() + R"(" placeholder="192.168.1.1">
        </div>
        <div class="form-group">
            <label>Primary DNS:</label>
            <input type="text" name="dns1" value=")" + currentConfig.dns1.toString() + R"(" placeholder="8.8.8.8">
        </div>
        <div class="form-group">
            <label>Secondary DNS:</label>
            <input type="text" name="dns2" value=")" + currentConfig.dns2.toString() + R"(" placeholder="8.8.4.4">
        </div>
        <button type="submit" class="btn">Save Network Configuration</button>
    </form>
    )";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleNetworkSave(AsyncWebServerRequest *request) {
    String message = "";
    
    if (request->hasParam("wifiSSID", true)) {
        String ssid = request->getParam("wifiSSID", true)->value();
        strncpy(currentConfig.wifiSSID, ssid.c_str(), MAX_SSID_LENGTH);
        currentConfig.wifiSSID[MAX_SSID_LENGTH] = '\0';
    }
    
    if (request->hasParam("wifiPassword", true)) {
        String pass = request->getParam("wifiPassword", true)->value();
        strncpy(currentConfig.wifiPassword, pass.c_str(), MAX_PASSWORD_LENGTH);
        currentConfig.wifiPassword[MAX_PASSWORD_LENGTH] = '\0';
    }
    
    currentConfig.dhcp = request->hasParam("dhcp", true);
    
    if (request->hasParam("staticIP", true)) {
        currentConfig.staticIP.fromString(request->getParam("staticIP", true)->value());
    }
    if (request->hasParam("netmask", true)) {
        currentConfig.netmask.fromString(request->getParam("netmask", true)->value());
    }
    if (request->hasParam("gateway", true)) {
        currentConfig.gateway.fromString(request->getParam("gateway", true)->value());
    }
    if (request->hasParam("dns1", true)) {
        currentConfig.dns1.fromString(request->getParam("dns1", true)->value());
    }
    if (request->hasParam("dns2", true)) {
        currentConfig.dns2.fromString(request->getParam("dns2", true)->value());
    }
    
    if (saveConfig(currentConfig)) {
        message = "Network configuration saved successfully!";
    } else {
        message = "Error saving configuration";
    }
    
    String html = FPSTR(HTML_HEADER);
    html += "<h2>Network Configuration</h2>";
    html += "<div class='" + String(message.startsWith("Error") ? "error" : "success") + "'>" + message + "</div>";
    html += "<button class='btn' onclick='location.href=\"/network\"'>Back to Network Config</button>";
    html += "<button class='btn' onclick='location.href=\"/\"'>Home</button>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleMqttPage(AsyncWebServerRequest *request) {
    if (!currentConfig.expertMode) {
        request->send(403, "text/html", "Expert mode required");
        return;
    }
    
    String html = FPSTR(HTML_HEADER);
    html += R"(
    <div class="expert-only">
        <h2>MQTT Configuration (Expert Mode)</h2>
        <form method="POST" action="/mqtt">
            <div class="form-group">
                <label>MQTT Server:</label>
                <input type="text" name="mqttServer" maxlength="32" value=")" + String(currentConfig.mqttServer) + R"(" required>
            </div>
            <div class="form-group">
                <label>MQTT Port:</label>
                <input type="number" name="mqttPort" min="1" max="65535" value=")" + String(currentConfig.mqttPort) + R"(" required>
            </div>
            <div class="form-group">
                <label>MQTT Username:</label>
                <input type="text" name="mqttUser" maxlength="32" value=")" + String(currentConfig.mqttUser) + R"(">
            </div>
            <div class="form-group">
                <label>MQTT Password:</label>
                <input type="password" name="mqttPass" maxlength="64" value=")" + String(currentConfig.mqttPass) + R"(">
            </div>
            <button type="submit" class="btn">Save MQTT Configuration</button>
        </form>
    </div>
    )";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleMqttSave(AsyncWebServerRequest *request) {
    if (!currentConfig.expertMode) {
        request->send(403, "text/html", "Expert mode required");
        return;
    }
    
    String message = "";
    
    if (request->hasParam("mqttServer", true)) {
        String server = request->getParam("mqttServer", true)->value();
        strncpy(currentConfig.mqttServer, server.c_str(), MAX_STRING_LENGTH);
        currentConfig.mqttServer[MAX_STRING_LENGTH] = '\0';
    }
    
    if (request->hasParam("mqttPort", true)) {
        currentConfig.mqttPort = request->getParam("mqttPort", true)->value().toInt();
    }
    
    if (request->hasParam("mqttUser", true)) {
        String user = request->getParam("mqttUser", true)->value();
        strncpy(currentConfig.mqttUser, user.c_str(), MAX_STRING_LENGTH);
        currentConfig.mqttUser[MAX_STRING_LENGTH] = '\0';
    }
    
    if (request->hasParam("mqttPass", true)) {
        String pass = request->getParam("mqttPass", true)->value();
        strncpy(currentConfig.mqttPass, pass.c_str(), MAX_PASSWORD_LENGTH);
        currentConfig.mqttPass[MAX_PASSWORD_LENGTH] = '\0';
    }
    
    if (saveConfig(currentConfig)) {
        message = "MQTT configuration saved successfully!";
    } else {
        message = "Error saving configuration";
    }
    
    String html = FPSTR(HTML_HEADER);
    html += "<h2>MQTT Configuration</h2>";
    html += "<div class='" + String(message.startsWith("Error") ? "error" : "success") + "'>" + message + "</div>";
    html += "<button class='btn' onclick='location.href=\"/mqtt\"'>Back to MQTT Config</button>";
    html += "<button class='btn' onclick='location.href=\"/\"'>Home</button>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleApPage(AsyncWebServerRequest *request) {
    String html = FPSTR(HTML_HEADER);
    html += R"(
    <h2>Access Point Configuration</h2>
    <form method="POST" action="/ap">
    )";
    
    if (currentConfig.expertMode) {
        html += R"(
        <div class="expert-only">
            <div class="form-group">
                <label>AP Name (Expert Mode):</label>
                <input type="text" name="apName" maxlength="32" value=")" + String(currentConfig.apName) + R"(">
            </div>
        </div>
        )";
    }
    
    html += R"(
        <div class="form-group">
            <label>AP Username:</label>
            <input type="text" name="apUser" maxlength="32" value=")" + String(currentConfig.apUser) + R"(" required>
        </div>
        <div class="form-group">
            <label>AP Password:</label>
            <input type="password" name="apPassword" maxlength="64" value=")" + String(currentConfig.apPassword) + R"(" required>
        </div>
        <button type="submit" class="btn">Save AP Configuration</button>
    </form>
    )";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleApSave(AsyncWebServerRequest *request) {
    String message = "";
    
    if (currentConfig.expertMode && request->hasParam("apName", true)) {
        String name = request->getParam("apName", true)->value();
        strncpy(currentConfig.apName, name.c_str(), MAX_SSID_LENGTH);
        currentConfig.apName[MAX_SSID_LENGTH] = '\0';
    }
    
    if (request->hasParam("apUser", true)) {
        String user = request->getParam("apUser", true)->value();
        strncpy(currentConfig.apUser, user.c_str(), MAX_STRING_LENGTH);
        currentConfig.apUser[MAX_STRING_LENGTH] = '\0';
    }
    
    if (request->hasParam("apPassword", true)) {
        String pass = request->getParam("apPassword", true)->value();
        strncpy(currentConfig.apPassword, pass.c_str(), MAX_PASSWORD_LENGTH);
        currentConfig.apPassword[MAX_PASSWORD_LENGTH] = '\0';
    }
    
    if (saveConfig(currentConfig)) {
        message = "Access Point configuration saved successfully!";
    } else {
        message = "Error saving configuration";
    }
    
    String html = FPSTR(HTML_HEADER);
    html += "<h2>Access Point Configuration</h2>";
    html += "<div class='" + String(message.startsWith("Error") ? "error" : "success") + "'>" + message + "</div>";
    html += "<button class='btn' onclick='location.href=\"/ap\"'>Back to AP Config</button>";
    html += "<button class='btn' onclick='location.href=\"/\"'>Home</button>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleSystemPage(AsyncWebServerRequest *request) {
    String html = FPSTR(HTML_HEADER);
    html += "<h2>System Configuration</h2>";
    html += "<form method='POST' action='/system'>";
    html += "<div class='form-group'>";
    html += "<input type='checkbox' name='expertMode' " + String(currentConfig.expertMode ? "checked" : "") + "> Enable Expert Mode";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Expert Mode Password:</label>";
    html += "<input type='password' name='expertPassword' placeholder='Enter expert password'>";
    html += "</div>";
    html += "<button type='submit' name='action' value='save' class='btn'>Save System Configuration</button>";
    html += "<button type='submit' name='action' value='reboot' class='btn btn-warning' onclick='return confirm(\"Are you sure you want to reboot?\")'>System Reboot</button>";
    html += "<button type='submit' name='action' value='factory-reset' class='btn btn-danger' onclick='return confirm(\"Are you sure you want to factory reset?\")'>Factory Reset</button>";
    html += "</form>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleSystemAction(AsyncWebServerRequest *request) {
    String message = "";
    String action = "";
    
    if (request->hasParam("action", true)) {
        action = request->getParam("action", true)->value();
    }
    
    if (action == "save") {
        bool expertModeRequested = request->hasParam("expertMode", true);
        String expertPassword = "";
        
        if (request->hasParam("expertPassword", true)) {
            expertPassword = request->getParam("expertPassword", true)->value();
        }
        
        const char* expectedPassword = EXPERT_MODE_PASSWORD;
        if (expertModeRequested && expertPassword != String(expectedPassword)) {
            message = "Error: Invalid expert mode password";
        } else {
            currentConfig.expertMode = expertModeRequested;
            if (saveConfig(currentConfig)) {
                message = "System configuration saved successfully!";
            } else {
                message = "Error saving configuration";
            }
        }
    } else if (action == "reboot") {
        message = "System is rebooting...";
        String rebootHtml = String(FPSTR(HTML_HEADER)) + "<h2>System Reboot</h2><p>" + message + "</p>" + String(HTML_FOOTER);
        request->send(200, "text/html", rebootHtml);
        delay(1000);
        ESP.restart();
        return;
    } else if (action == "factory-reset") {
        factoryReset();
        message = "Factory reset completed. System is rebooting...";
        String resetHtml = String(FPSTR(HTML_HEADER)) + "<h2>Factory Reset</h2><p>" + message + "</p>" + String(HTML_FOOTER);
        request->send(200, "text/html", resetHtml);
        delay(1000);
        ESP.restart();
        return;
    }
    
    String html = FPSTR(HTML_HEADER);
    html += "<h2>System Configuration</h2>";
    html += "<div class='" + String(message.startsWith("Error") ? "error" : "success") + "'>" + message + "</div>";
    html += "<button class='btn' onclick='location.href=\"/system\"'>Back to System Config</button>";
    html += "<button class='btn' onclick='location.href=\"/\"'>Home</button>";
    html += HTML_FOOTER;
    
    request->send(200, "text/html", html);
}

void handleApiStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["networkId"] = currentConfig.networkId;
    doc["nodeId"] = currentConfig.nodeId;
    doc["expertMode"] = currentConfig.expertMode;
    doc["dhcp"] = currentConfig.dhcp;
    doc["wifiSSID"] = currentConfig.wifiSSID;
    doc["uptime"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    request->send(200, "application/json", jsonString);
}

void handleApiReboot(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(1000);
    ESP.restart();
}

void handleApiFactoryReset(AsyncWebServerRequest *request) {
    factoryReset();
    request->send(200, "application/json", "{\"status\":\"factory-reset-complete\"}");
    delay(1000);
    ESP.restart();
}

void handleWebRequests() {
    if (configModeActive) {
        dnsServer.processNextRequest();
    }
}

void enterConfigurationMode() {
    debugLog("Entering configuration mode");
    
    // Load current configuration
    if (!loadConfig(currentConfig)) {
        debugLog("Using default configuration for config mode");
        currentConfig = defaultConfig;
    }
    
    startCaptivePortal();
    
    // Configuration mode main loop
    while (configModeActive) {
        handleWebRequests();
        delay(10);
    }
}
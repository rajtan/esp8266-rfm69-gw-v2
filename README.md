# ESP8266 RFM69 Gateway v2

A comprehensive IoT gateway that bridges RFM69 radio communication with MQTT, built on ESP8266 using the Arduino framework.

## Features

### Core Functionality
- **Bidirectional Gateway**: Seamlessly forwards messages between RFM69 radio network and MQTT broker
- **Web-based Configuration**: User-friendly captive portal for all settings management
- **EEPROM Configuration Storage**: Persistent configuration with checksum validation
- **Expert Mode**: Advanced settings protection with password authentication
- **Factory Reset**: Complete configuration reset capability

### Dual Operating Modes

#### Normal Mode (Gateway Operation)
- RFM69 radio communication with multiple nodes
- WiFi connectivity with DHCP or static IP
- MQTT client with automatic reconnection
- Real-time message forwarding and protocol conversion
- Status reporting and remote management

#### Configuration Mode (Captive Portal)
- Automatic access point creation
- Web-based configuration interface
- Organized configuration pages (Radio, Network, MQTT, AP, System)
- Expert mode for advanced settings
- Live configuration validation

## Hardware Requirements

- **ESP8266** (ESP12E module recommended)
- **RFM69HCW** radio module (915MHz or 868MHz)
- **Connections**:
  - RFM69 CS → GPIO15 (D8)
  - RFM69 IRQ → GPIO4 (D2)
  - RFM69 RST → GPIO2 (D4)
  - Configuration GPIO → GPIO0 (Boot button)

## Configuration Parameters

### Radio Configuration
- Network ID (1-255)
- Node ID (1-255) - Expert mode only, default: 1
- Encryption Key (16 characters)
- Radio Power (0-31) - Expert mode only, default: 31

### Network Configuration
- WiFi SSID and Password
- DHCP enable/disable
- Static IP Configuration (IP, Netmask, Gateway, DNS1, DNS2)

### MQTT Configuration (Expert Mode Only)
- MQTT Server hostname/IP
- MQTT Port (default: 1883)
- MQTT Username and Password

### Access Point Configuration
- AP Name (Expert mode only, default: "ESP8266-Gateway")
- AP Username and Password for captive portal

### System Configuration
- Expert Mode enable/disable
- Expert Mode Password (default: "admin123")

## Installation & Setup

### 1. Hardware Assembly
Connect the RFM69 module to your ESP8266 according to the pin definitions above.

### 2. Software Setup
```bash
# Clone and build with PlatformIO
git clone <repository-url>
cd esp8266-rfm69-gw-v2
pio run --target upload
```

### 3. Initial Configuration
1. Power on the device
2. Hold the BOOT button (GPIO0) for 5 seconds during startup
3. Connect to the "ESP8266-Gateway" WiFi network
4. Open a web browser (captive portal should auto-redirect)
5. Login with default credentials (admin/password123)
6. Configure your settings through the web interface

## Usage

### MQTT Topics

The gateway uses the following MQTT topic structure:

```
gateway/{nodeId}/status          # Gateway status reports
gateway/{nodeId}/radio/received/{senderId}  # Incoming radio messages
gateway/{nodeId}/command/send    # Send radio messages
gateway/{nodeId}/command/status  # Request status update
gateway/{nodeId}/command/reboot  # Remote reboot
gateway/{nodeId}/response/send   # Send command responses
```

### Radio Message Format

Messages are forwarded as JSON:
```json
{
  "timestamp": 12345,
  "senderId": 2,
  "targetId": 1,
  "rssi": -65,
  "message": "sensor_data",
  "data": { ... }  // If message is valid JSON
}
```

### Sending Radio Messages via MQTT

Publish to `gateway/{nodeId}/command/send`:
```json
{
  "nodeId": 2,
  "message": "Hello Node 2",
  "ack": true
}
```

## Configuration Mode Activation

Configuration mode can be activated by:
1. Holding GPIO0 (BOOT button) LOW for 5 seconds during startup
2. EEPROM checksum validation failure (automatic)
3. WiFi connection failure in normal mode (automatic fallback)

## Build Configuration

Customize build-time parameters in `platformio.ini`:

```ini
build_flags = 
    -DRFM69_FREQUENCY=915.0        ; Radio frequency (MHz)
    -DENABLE_EXPERT_CONFIG=false   ; Default expert mode state
    -DCONF_GPIO_NUM=0              ; Configuration GPIO pin
    -DCONF_GPIO_HOLD_MS=5000       ; Hold time (milliseconds)
    -DCONF_GPIO_HOLD_STATE=LOW     ; Active state
    -DEXPERT_MODE_PASSWORD="admin123"  ; Expert mode password
```

## Default Configuration

The gateway ships with these default values:
- Network ID: 1
- Node ID: 1 (Gateway)
- Encryption Key: "samplekey12345"
- WiFi: "your_wifi_ssid" / "your_wifi_password"
- MQTT: "mqtt.broker.com:1883"
- AP Credentials: "admin" / "password123"

## Troubleshooting

### Cannot Connect to Gateway
- Ensure the configuration GPIO was held during startup
- Look for "ESP8266-Gateway" WiFi network
- Try factory reset (System page → Factory Reset)

### Radio Communication Issues
- Verify RFM69 wiring connections
- Check frequency settings match other nodes
- Ensure encryption keys are identical across network
- Verify network ID consistency

### MQTT Connection Problems
- Check MQTT broker accessibility
- Verify credentials in expert mode
- Confirm network connectivity
- Review MQTT broker logs

### Configuration Not Saving
- Check EEPROM functionality
- Verify checksum calculations
- Try factory reset and reconfigure

## Development

### Project Structure
```
├── platformio.ini      # Build configuration
├── include/
│   └── config.h        # Configuration structures
├── src/
│   ├── main.cpp        # Application entry point
│   ├── config.cpp      # EEPROM management
│   ├── web_config.cpp  # Captive portal & web interface
│   └── gateway.cpp     # Normal mode operations
├── lib/                # Custom libraries
└── test/               # Unit tests
```

### Adding Features
1. Configuration variables: Update `GatewayConfig` struct in `config.h`
2. Web interface: Add pages in `web_config.cpp`
3. MQTT handlers: Extend `gateway.cpp` functions
4. Radio protocols: Modify message parsing logic

## License

This project is open source. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## Support

For issues and feature requests, please use the GitHub issue tracker.
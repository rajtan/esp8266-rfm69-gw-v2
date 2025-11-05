# Copilot Instructions for ESP8266-RFM69-Gateway v2

## Project Overview
This is a PlatformIO-based firmware project for an ESP8266 (ESP12E) microcontroller intended to act as an RFM69 radio gateway. The project is currently in early development with placeholder Arduino framework code.

## Architecture & Structure
- **Target Platform**: ESP8266 (ESP12E module) using Arduino framework
- **Build System**: PlatformIO with espressif8266 platform
- **Code Organization**: Standard PlatformIO structure with `src/`, `lib/`, `include/`, and `test/` directories

## Development Workflow

### Building & Flashing
```bash
# Build the project
pio run

# Upload to device (ensure correct serial port)
pio run --target upload

# Monitor serial output
pio device monitor
```

### Key Files & Directories
- `platformio.ini`: Core project configuration targeting ESP12E board
- `src/main.cpp`: Main application entry point (currently placeholder)
- `lib/`: Place custom libraries here (e.g., RFM69 radio driver, protocol handlers)
- `include/`: Project-specific header files
- `.vscode/extensions.json`: Recommends PlatformIO IDE extension

## Project-Specific Patterns

### Expected Development Path
Based on the project name (`esp8266-rfm69-gw-v2`), this will likely involve:
- RFM69 radio communication library integration
- WiFi gateway functionality for bridging radio to network
- Message routing/forwarding between RFM69 nodes and WiFi/Internet
- Configuration management for radio parameters and network settings

### Library Management
- Use `lib/` for project-specific libraries (RFM69 drivers, protocol handlers)
- PlatformIO's Library Dependency Finder automatically resolves external dependencies
- Consider creating modular libraries for radio management, network handling, and message processing

### ESP8266-Specific Considerations
- Limited flash/RAM - optimize memory usage
- WiFi capabilities should be leveraged for gateway functionality  
- Use ESP8266-specific Arduino libraries for WiFi, OTA updates, and hardware interfaces
- Consider deep sleep modes for power efficiency if battery-powered

## Configuration Guidelines
- Modify `platformio.ini` to add required libraries as dependencies
- Use build flags for compile-time configuration (radio frequency, network settings)
- Consider environment-specific configurations for development vs production builds

## Testing Strategy
- Use PlatformIO's unit testing framework in `test/` directory
- Consider hardware-in-the-loop testing for radio communication
- Mock external dependencies for isolated unit tests

## Common Tasks
- **Adding RFM69 Library**: Add to `lib_deps` in `platformio.ini` or place custom implementation in `lib/`
- **WiFi Configuration**: Implement in `src/main.cpp` using ESP8266WiFi library
- **Serial Debugging**: Use `Serial.println()` and monitor with `pio device monitor`
- **OTA Updates**: Configure in `platformio.ini` for wireless firmware updates
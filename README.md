# IoT-Based Smart Lock and Environmental Monitoring System

## Overview
This project implements an IoT-based smart lock system integrated with environmental monitoring sensors. It uses an ESP32 microcontroller to connect to a WebSocket server and an HTTP API for real-time data exchange. The system includes features such as remote locking/unlocking, temperature and humidity sensing, dust concentration measurement, and sound detection.

## Features
- **Smart Lock Control:** Remote locking and unlocking via WebSocket commands.
- **Environmental Monitoring:** Measures temperature, humidity, dust concentration, and sound detection.
- **WebSocket Communication:** Ensures real-time data exchange.
- **HTTP API Integration:** Sends sensor data to a remote server.
- **Dual-Core Processing:** Utilizes FreeRTOS tasks for efficient execution.

## Hardware Requirements
- ESP32 Development Board
- DHT22 Temperature & Humidity Sensor
- Sound Sensor
- Dust Sensor
- PIR Motion Sensor
- Electromagnetic Lock
- LEDs and Resistors

## Software Requirements
- Arduino IDE
- ESP32 Board Support Package
- Required Libraries:
  - WiFi
  - WebSocketsClient
  - HTTPClient
  - ArduinoJson
  - DHT

## Setup and Installation
1. **Clone the Repository:**
   
sh
   git clone https://github.com/yourusername/SmartLock-IoT.git
   cd SmartLock-IoT

2. **Install Dependencies:**
   Ensure you have installed all required libraries in the Arduino IDE.
3. **Configure WiFi Credentials:**
   Modify ssid and password variables in the code to match your network settings.
4. **Flash the Code:**
   Upload the code to the ESP32 using Arduino IDE.
5. **Monitor Serial Output:**
   Open the Serial Monitor to check connection status and sensor readings.

## Usage
- The system continuously monitors environmental parameters and sends data to the server.
- WebSocket commands onlock and offlock control the electromagnetic lock.
- The system periodically sends a ping message to keep the WebSocket connection active.

## API Endpoints
- **Device Authentication:**
  
http
  POST /api/devices/authenticate

- **Send Sensor Data:**
  
http
  POST /api/datacollection/event


## Future Improvements
- Implement mobile app integration.
- Add an OLED display for real-time feedback.
- Enhance security with encrypted communication.

## License
This project is licensed under the MIT License.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss your ideas.

## Contact
For inquiries, reach out via email: your.email@example.com

# 💊 Smart MediBox
An ESP32-based smart medicine box with **IoT integration, alarm management, MQTT–Node-RED dashboard integration, NTP sync, environmental monitoring, and adaptive servo shielding**.  Developed as part of the *EN2853: Embedded Systems and Applications* module.


## 🚀 Overview
Smart MediBox helps manage medications by combining:
- Precise **time & alarm scheduling**
- **IoT connectivity** with Node-RED & MQTT
- **Environmental safety monitoring** (temperature & humidity)
- **Adaptive servo shielding** to secure medications
- Multiple **fallback modes** (Wi-Fi, manual, IoT)


## ✨ Features

### ✅ Core Capabilities
- Set timezone & sync with **NTP server**
- Set up to **2 alarms**, view active alarms, and delete alarms
- Alarm **ring & stop** with buzzer + button
- **Manual time update** when Wi-Fi unavailable
- Display current time on OLED (with **partial refresh** for performance)
- **Button press handling** with debounce & timeout
- **Battery percentage monitoring**

### 📡 IoT & Dashboard
- MQTT + Node-RED dashboard integration
- LDR sampling & data visualization
- Temperature & humidity monitoring with DHT11
- Warnings when thresholds are exceeded
- Servo angle control via dashboard sliders
- **UTC update from dashboard**
- Battery usage reporting to dashboard

### ⚙️ Advanced Enhancements
- **Multimode support**: Manual / Wi-Fi / IoT
- Two digitizer function
- Dual printing styles for UI
- “Cancel” button repurposed as **Back/Menu**
- Optimized **servo control** using formula-based computation



## 🏗️ System Architecture

### Modules
- **Time & Alarm Management**
- **Environmental Monitoring (DHT11, LDR)**
- **UI & Menu Navigation (OLED + buttons)**
- **Servo Control** with adaptive shielding
- **IoT Communication** (MQTT + Node-RED)
- **Power Management** (Battery %)




## 🖥️ Dashboard

- Node-RED dashboard provides:
  - Light intensity visualization
  - Temperature & humidity monitoring
  - Servo control sliders
  - Battery usage stats
  - Alarm & UTC configuration

📌 Import flow from [`dashboard/medibox_dashboard.json`](dashboard/medibox_dashboard.json).



## 🛠️ Getting Started

### Requirements
- ESP32 Dev Kit
- DHT11 Sensor
- LDR Module
- Slider Switch
- OLED Display (128x64)
- Servo Motor
- Push Buttons
- Buzzer
- Li-ion Battery + Charger Module

### Arduino Libraries
See [`libraries.txt`](libraries.txt).

### Setup
1. Clone this repository  
   ```bash
   git clone https://github.com/YOUR_USERNAME/Smart-MediBox.git
2. Open src/SmartMediBox.ino in Arduino IDE / PlatformIO
3. Install required libraries
4. Upload to ESP32
5. Import Node-RED flow (dashboard/medibox_dashboard.json)
6. Connect to Wi-Fi and launch dashboard

### 📸 Screenshot
	
### 📝 Project Context

This project was completed as part of the Embedded Systems & Applications Module (2025).
It integrates academic assignment criteria (alarms, NTP sync, LDR/servo, dashboard) with extra enhancements (battery monitoring, fallback modes, optimized UI).

### 📌 Future Work
- Non-volatile memory for alarm persistence
- Enhanced UI themes
- Mobile app integration
- Advanced data logging & analytics

### 📄 License

This project is licensed under the MIT License.
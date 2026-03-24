**IoT Smart Health & Fall Emergency System (ESP32)**

A robust, wearable IoT solution designed for real-time tracking of vital signs and automated emergency detection. This system leverages multi-core processing to ensure high-frequency sensor sampling remains uninterrupted by network latency and cloud synchronization.

🚀 Key Features
* **Real-time Vitals:** Continuous monitoring of Heart Rate (BPM), Body Temperature, and Environmental conditions.
* **Intelligent Fall Detection:** Dual-stage verification using a 3-axis accelerometer and tilt sensor to minimize false positives.
* **Dual-Core Execution:** Utilizes **FreeRTOS** to pin critical sensor tasks to Core 0 and cloud communication to Core 1.
* **Self-Healing Architecture:** Integrated **Watchdog Timer (WDT)** to automatically recover the system in case of network or software freezes.
* **Data Smoothing:** Implements a **Moving Average Filter** to provide stable, noise-free sensor readings.
* **Cloud Dashboard:** Real-time data synchronization with **Firebase RTDB**.

🛠️ Hardware Components
* **Microcontroller:** ESP32 (Dual-core)
* **Sensors:**
    * **MAX30105:** High-sensitivity Pulse Oximeter and Heart-Rate sensor.
    * **DS18B20:** High-accuracy digital thermometer for body temperature.
    * **MPU6050:** 6-axis Motion Tracking (Accelerometer & Gyro) for fall detection.
    * **DHT22:** Ambient Temperature & Humidity sensor.
* **User Interface:**
    * **0.96" OLED (SH1106):** Local data visualization.
    * **RGB LED & Buzzer:** Visual and audible emergency indicators.

💻 Technical Implementation

### 1. Multi-Core Task Management
To prevent the high latency of WiFi/Firebase updates from interfering with the time-sensitive sampling required for heart rate detection, the system uses **FreeRTOS Task Pinning**:
* **Core 0 (Sensor Task):** Dedicated to high-speed polling of the MAX30105 and MPU6050.
* **Core 1 (Cloud Task):** Handles WiFi connectivity, JSON payload construction, and Firebase HTTPS transmission.

### 2. Emergency & Fall Logic
The system uses a 5-second verification window to confirm emergencies:
1. **Impact Detection:** Triggered if total acceleration exceeds a threshold of `10.0G`.
2. **Tilt Verification:** The system checks the orientation via the `TILT_PIN`. If the user remains in a fallen state for 5 seconds, an `EMERGENCY` status is broadcast to the cloud.

### 3. Reliability & Safety
* **Watchdog Timer:** The `esp_task_wdt` ensures the system resets if the main loop hangs, maintaining 24/7 reliability.
* **Sensor Health Check:** Integrated `checkSensors()` routine to monitor hardware connectivity and report errors via Serial.

📊 Health State Classification
| State | Criteria | Indicators |
| :--- | :--- | :--- |
| **STABLE** | Normal Heart Rate & Temperature | Green LED |
| **DANGER** | Temp > 38.5°C or HR > 140 / < 40 | Red LED + Buzzer |
| **FALL!** | High Impact + 5s Tilt Confirmation | Red LED + Constant Buzzer |
| **SYNCING** | Finger detected, calculating BPM | Blinking Blue LED |

🔧 Setup & Installation
1.  **Libraries Required:**
    * `Firebase_ESP_Client`
    * `U8g2`
    * `Adafruit_MPU6050`
    * `DallasTemperature`
    * `DHT sensor library`
2.  **Hardware Pins:**
    * **I2C:** SDA (33), SCL (32)
    * **DHT22:** Pin 4
    * **Buzzer:** Pin 18
    * **RGB LEDs:** Pins 13, 14, 25
3.  **Deployment:** Update the `WIFI_SSID`, `WIFI_PASSWORD`, and `API_KEY` in the source code before flashing to the ESP32.

--Developed as part of the IoT & Embedded Systems coursework at NIBM.--

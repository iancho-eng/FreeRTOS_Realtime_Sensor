# 📊 FreeRTOS Real-Time Sensor Dashboard

A multitasking embedded application on the **ESP32-WROOM-32E** using **FreeRTOS**, running two concurrent tasks pinned to separate cores — an LED blink task and a DHT11 temperature/humidity sensor task — with live readings streamed over UART.

---

## ✨ Features

- **True dual-core parallelism**: LED task pinned to Core 0, DHT11 sensor task pinned to Core 1
- **Non-blocking scheduling**: `vTaskDelay()` yields CPU control between activations instead of busy-waiting
- **Precise timing**: 1-second LED blink period, 2-second sensor sample period (matches DHT11 hardware limit)
- **Live UART output**: temperature (°C / °F), humidity (%), and computed heat index at 115200 baud
- **Sensor fault detection**: detects and reports DHT11 read failures gracefully

---

## 🖥️ Serial Monitor Output

```
╔══════════════════════════════╗
║  FreeRTOS Sensor Dashboard   ║
╚══════════════════════════════╝
[LED Task]  Started on core 0
[DHT Task]  Started on core 1
────────────────────────────
  Temperature : 24.0 °C  /  75.2 °F
  Humidity    : 55.0 %
  Heat index  : 23.8 °C
────────────────────────────
```

---

## 🔧 Hardware

| Component | Part | Notes |
|-----------|------|-------|
| Microcontroller | ESP32-WROOM-32E | Dual-core Xtensa LX6 @ 240 MHz |
| Sensor | DHT11 | Temperature & humidity |
| LED | Any color | GPIO2 (built-in LED on most boards) |
| Resistor (LED) | 330 Ω | Current limiting for LED |
| Resistor (DHT11) | 10 kΩ | Pull-up on DHT11 data line |
| Breadboard + jumper wires | — | For prototyping |

---

## 🔌 Wiring

```
ESP32-WROOM-32E
┌──────────────────────────────────────────┐
│                                          │
│  3.3V ──────────────── DHT11 VCC         │
│                    ┌── DHT11 DATA        │
│  GPIO4 ────────────┤                     │
│                    └── 10kΩ ── 3.3V      │
│  GND  ──────────────── DHT11 GND         │
│                                          │
│  GPIO2 ── 330Ω ── LED (+)                │
│  GND   ────────── LED (-)                │
│                                          │
└──────────────────────────────────────────┘
```

> ⚠️ The 10 kΩ pull-up resistor on the DHT11 data line is required — without it the line floats and you'll get read errors.

---

## 💻 Software & Setup

### Prerequisites

- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- ESP32 board support package — add this URL under **File → Preferences → Additional Boards Manager URLs**:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
  Then install via **Tools → Board → Boards Manager → search "esp32"**
- **Adafruit DHT sensor library** — install via Library Manager (also install *Adafruit Unified Sensor* when prompted)

### Upload Steps

1. Open the `.ino` file in Arduino IDE
2. Set **Tools → Board → ESP32 Dev Module**
3. Set **Tools → Port** to your ESP32's COM port
4. Click **Upload**
5. If you see `Connecting........_____`, hold **FLASH**, tap **RST** once, then release **FLASH**
6. Open **Serial Monitor** at **115200 baud**

---

## 🏗️ FreeRTOS Architecture

```
FreeRTOS Scheduler
├── Task 1: LED Blink      [Core 0]  priority 1  period: 1000 ms
│     └── digitalWrite toggle → vTaskDelay(1000ms)
│
└── Task 2: DHT11 Sensor   [Core 1]  priority 1  period: 2000 ms
      └── dht.readTemperature/Humidity → Serial.println → vTaskDelay(2000ms)
```

Key APIs used:

- `xTaskCreatePinnedToCore()` — spawns each task on a specific core
- `vTaskDelay()` with `pdMS_TO_TICKS()` — yields CPU between activations
- `Serial.println()` — UART output to Serial Monitor at 115200 baud

---

## ⚙️ Key Design Decisions

**Why pin tasks to separate cores?**
The ESP32's dual Xtensa LX6 cores allow true hardware parallelism. By pinning the LED task to Core 0 and the sensor task to Core 1, neither task can block the other — the sensor's 2-second read cycle has zero impact on the 1-second LED timing.

**Why `vTaskDelay()` instead of `delay()`?**
`delay()` busy-waits and blocks the entire core. `vTaskDelay()` suspends the task and returns control to the FreeRTOS scheduler, allowing other tasks (or the idle task) to run during the wait period.

**Why 2 seconds for DHT11 sampling?**
The DHT11 has a hardware-enforced minimum inter-sample period of ~2 seconds. Sampling faster returns stale or invalid readings.

---

## ⚠️ Limitations

- **DHT11 accuracy**: ±2°C temperature, ±5% humidity — use a DHT22 for higher precision
- **No inter-task communication**: tasks currently operate independently; a shared queue/mutex could be added to aggregate data
- **No display output**: results are UART-only; an I2C OLED (SSD1306) would be a natural next step

---

## 📄 License

This project is open-source under the [MIT License](LICENSE).

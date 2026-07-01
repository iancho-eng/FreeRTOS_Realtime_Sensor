// ============================================================
//  FreeRTOS Real-Time Sensor Dashboard
//  Hardware : ESP32-WROOM-32E
//  Sensors  : DHT11 (GPIO4), LED (GPIO2)
//  Tasks    : LED blink @ 1s | DHT11 read @ 2s via UART
// ============================================================
//  Library required: "DHT sensor library" by Adafruit
//  Install via: Arduino IDE → Library Manager → search "DHT sensor library"
//  Also install its dependency: "Adafruit Unified Sensor"
// ============================================================

#include <Arduino.h>
#include "DHT.h"

// ── Pin definitions ──────────────────────────────────────────
#define LED_PIN     2       // Built-in LED on most ESP32 DevKitC boards
#define DHT_PIN     4       // DHT11 data line → GPIO4
#define DHT_TYPE    DHT11

// ── FreeRTOS task handles (optional, useful for debugging) ───
TaskHandle_t ledTaskHandle   = NULL;
TaskHandle_t dhtTaskHandle   = NULL;

// ── DHT sensor object ────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
//  Task 1: LED Blink
//  Toggles the LED every 1 second independently of all other tasks.
// ============================================================
void ledBlinkTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);

  Serial.println("[LED Task] Started on core " + String(xPortGetCoreID()));

  while (true) {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));   // ON for 1 second

    digitalWrite(LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));   // OFF for 1 second
  }
}

// ============================================================
//  Task 2: DHT11 Sensor Read
//  Reads temperature and humidity every 2 seconds,
//  streams results over UART to the Serial Monitor.
// ============================================================
void dhtSensorTask(void *pvParameters) {
  dht.begin();

  // Small startup delay so DHT11 can initialise (it needs ~1s after power-on)
  vTaskDelay(pdMS_TO_TICKS(2000));

  Serial.println("[DHT Task] Started on core " + String(xPortGetCoreID()));

  while (true) {
    float humidity    = dht.readHumidity();
    float tempC       = dht.readTemperature();       // Celsius
    float tempF       = dht.readTemperature(true);   // Fahrenheit

    // Check for failed readings
    if (isnan(humidity) || isnan(tempC)) {
      Serial.println("[DHT Task] ERROR: Failed to read from DHT11 sensor!");
      Serial.println("           Check wiring: VCC→3.3V, GND→GND, DATA→GPIO4");
      Serial.println("           Ensure 10kΩ pull-up resistor on DATA line.");
    } else {
      // Print formatted output to Serial Monitor
      Serial.println("────────────────────────────");
      Serial.printf("  Temperature : %.1f °C  /  %.1f °F\n", tempC, tempF);
      Serial.printf("  Humidity    : %.1f %%\n", humidity);

      // Heat index (feels-like temperature)
      float heatIndex = dht.computeHeatIndex(tempC, humidity, false);
      Serial.printf("  Heat index  : %.1f °C\n", heatIndex);
      Serial.println("────────────────────────────");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds before next reading
  }
}

// ============================================================
//  setup() — runs once on boot
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);  // Let Serial settle

  Serial.println("\n╔══════════════════════════════╗");
  Serial.println("║  FreeRTOS Sensor Dashboard   ║");
  Serial.println("╚══════════════════════════════╝");
  Serial.println("Spawning tasks...\n");

  // ── Create LED Blink Task ──────────────────────────────────
  // Parameters: function, name, stack size (bytes), param, priority, handle, core
  xTaskCreatePinnedToCore(
    ledBlinkTask,     // Task function
    "LED_Blink",      // Task name (for debugging)
    1024,             // Stack size in bytes (1KB is plenty for a blink task)
    NULL,             // No parameters passed
    1,                // Priority (1 = low, higher number = higher priority)
    &ledTaskHandle,   // Task handle
    0                 // Pin to Core 0
  );

  // ── Create DHT11 Sensor Task ───────────────────────────────
  xTaskCreatePinnedToCore(
    dhtSensorTask,    // Task function
    "DHT11_Sensor",   // Task name
    4096,             // Stack size (4KB — DHT library + Serial.printf need this much)
    NULL,             // No parameters
    1,                // Same priority as LED task — scheduler runs both fairly
    &dhtTaskHandle,   // Task handle
    1                 // Pin to Core 1 (ESP32 has 2 cores — spread the load)
  );

  Serial.println("Both tasks running. Open Serial Monitor at 115200 baud.\n");
}

// ============================================================
//  loop() — intentionally empty
//  FreeRTOS scheduler owns execution from here.
// ============================================================
void loop() {
  // Nothing here — FreeRTOS handles all scheduling.
  // vTaskDelete(NULL) would delete this task entirely if desired.
  vTaskDelay(pdMS_TO_TICKS(10000));
}

/*
  UWB Tag Node — ESP32
  Configured as a mobile tag; exchanges ranging messages with
  the anchor and reports distance + RX power over Serial.

  FreeRTOS architecture:
    - rangingTask (core 1): runs DW1000Ranging.loop(), handles all UWB callbacks
    - statusTask  (core 0): periodic heartbeat log so the Serial monitor shows
                            the tag is alive even when no anchor is in range
*/

#include <SPI.h>
#include "DW1000Ranging.h"

// ── Tag MAC address ──────────────────────────────────────────────────────────
const char TAG_ADDR[] = "7D:00:22:EA:82:60:3B:9C";

// ── SPI pins (DW1000) ────────────────────────────────────────────────────────
const uint8_t PIN_SCK  = 18;
const uint8_t PIN_MISO = 19;
const uint8_t PIN_MOSI = 23;
const uint8_t PIN_RST  = 27;
const uint8_t PIN_IRQ  = 34;
const uint8_t PIN_CS   = 4;

// ── Shared state (protected by mutex) ───────────────────────────────────────
struct RangeData {
    float    range;
    float    rxPower;
    uint16_t addr;
    bool     anchorActive;
};

static volatile RangeData rangeData  = {0.0f, 0.0f, 0x0000, false};
static SemaphoreHandle_t  rangeMutex = nullptr;

// ── FreeRTOS task handles ────────────────────────────────────────────────────
static TaskHandle_t hRangingTask = nullptr;
static TaskHandle_t hStatusTask  = nullptr;

// ── Forward declarations ─────────────────────────────────────────────────────
void onNewRange();
void onNewDevice(DW1000Device *device);
void onInactiveDevice(DW1000Device *device);
void rangingTask(void *pvParameters);
void statusTask(void *pvParameters);

// ────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // SPI + DW1000 init
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_CS, PIN_IRQ);

    DW1000Ranging.attachNewRange(onNewRange);
    DW1000Ranging.attachNewDevice(onNewDevice);
    DW1000Ranging.attachInactiveDevice(onInactiveDevice);

    // Optional: uncomment to enable built-in range smoothing filter
    // DW1000Ranging.useRangeFilter(true);

    DW1000Ranging.startAsTag(TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    Serial.println(F("[INFO] Tag ready"));

    // Create mutex before starting tasks
    rangeMutex = xSemaphoreCreateMutex();
    configASSERT(rangeMutex);

    // rangingTask on core 1 — keeps DW1000 polling on the same core as setup
    xTaskCreatePinnedToCore(
        rangingTask,   // task function
        "ranging",     // name (for debugging)
        4096,          // stack size (bytes)
        nullptr,       // parameters
        2,             // priority (higher = more urgent)
        &hRangingTask, // handle
        1              // core
    );

    // statusTask on core 0 — heartbeat/logging, easy to extend later
    xTaskCreatePinnedToCore(
        statusTask,
        "status",
        2048,          // smaller stack — this task does very little
        nullptr,
        1,             // lower priority than ranging
        &hStatusTask,
        0
    );
}

void loop()
{
    // Empty — all work is in FreeRTOS tasks
    vTaskDelete(nullptr);
}

// ── FreeRTOS tasks ───────────────────────────────────────────────────────────

void rangingTask(void *pvParameters)
{
    for (;;) {
        DW1000Ranging.loop();
        // Yield briefly so the idle task can run (feeds the watchdog)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void statusTask(void *pvParameters)
{
    RangeData local;  // local snapshot read under mutex each iteration

    for (;;) {
        if (xSemaphoreTake(rangeMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            local = rangeData;
            xSemaphoreGive(rangeMutex);
        }

        if (!local.anchorActive) {
            Serial.println(F("[STATUS] searching for anchor..."));
        }

        // Heartbeat every 2 s — only prints when no anchor is in range
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ── DW1000 callbacks (called from rangingTask context) ───────────────────────

void onNewRange()
{
    float    range   = DW1000Ranging.getDistantDevice()->getRange();
    float    rxPower = DW1000Ranging.getDistantDevice()->getRXPower();
    uint16_t addr    = DW1000Ranging.getDistantDevice()->getShortAddress();

    Serial.print("[RANGE] anchor=0x");
    Serial.print(addr, HEX);
    Serial.print("  dist=");
    Serial.print(range, 2);
    Serial.print(" m  rx=");
    Serial.print(rxPower, 1);
    Serial.println(" dBm");

    if (xSemaphoreTake(rangeMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        rangeData.range         = range;
        rangeData.rxPower       = rxPower;
        rangeData.addr          = addr;
        rangeData.anchorActive  = true;
        xSemaphoreGive(rangeMutex);
    }
}

void onNewDevice(DW1000Device *device)
{
    Serial.print("[FOUND] anchor: 0x");
    Serial.println(device->getShortAddress(), HEX);

    if (xSemaphoreTake(rangeMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        rangeData.anchorActive = true;
        xSemaphoreGive(rangeMutex);
    }
}

void onInactiveDevice(DW1000Device *device)
{
    Serial.print("[LOST] anchor: 0x");
    Serial.println(device->getShortAddress(), HEX);

    if (xSemaphoreTake(rangeMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        rangeData.anchorActive = false;
        xSemaphoreGive(rangeMutex);
    }
}

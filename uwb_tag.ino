/*
  UWB Tag Node — ESP32
  Configured as a mobile tag; exchanges ranging messages with
  the anchor and reports distance + RX power over Serial.
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

// ── Forward declarations ─────────────────────────────────────────────────────
void onNewRange();
void onNewDevice(DW1000Device *device);
void onInactiveDevice(DW1000Device *device);

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
}

void loop()
{
    DW1000Ranging.loop();
}

// ── Callbacks ────────────────────────────────────────────────────────────────

void onNewRange()
{
    float range   = DW1000Ranging.getDistantDevice()->getRange();
    float rxPower = DW1000Ranging.getDistantDevice()->getRXPower();
    uint16_t addr = DW1000Ranging.getDistantDevice()->getShortAddress();

    Serial.print("[RANGE] anchor=0x");
    Serial.print(addr, HEX);
    Serial.print("  dist=");
    Serial.print(range, 2);
    Serial.print(" m  rx=");
    Serial.print(rxPower, 1);
    Serial.println(" dBm");
}

void onNewDevice(DW1000Device *device)
{
    Serial.print("[FOUND] anchor: 0x");
    Serial.println(device->getShortAddress(), HEX);
}

void onInactiveDevice(DW1000Device *device)
{
    Serial.print("[LOST] anchor: 0x");
    Serial.println(device->getShortAddress(), HEX);
}

/*
 * FITALIS — Smart Medicine Reminder
 * ===============================================================
 * Board : ESP32 DevKit V1
 * Build : Arduino IDE 2.x  (board: "ESP32 Dev Module")
 *         or PlatformIO (env:esp32dev)
 *
 * WHAT IT DOES
 *   - Stores medicine schedules in flash (survive power loss).
 *   - Rings an active buzzer at each scheduled time.
 *   - Serves a local web dashboard for adding/editing schedules.
 *   - Keeps time via NTP; the dashboard can set the clock when there
 *     is no internet (AP mode).
 *   - Works with no WiFi at all: schedules stay in flash and the
 *     alarm still fires.
 *
 * HARDWARE (see include/config.h to change pins)
 *   Buzzer (+)  -> GPIO 25
 *   Buzzer (-)  -> GND
 *   Button      -> GPIO 27 and GND   (acknowledge: "I took my medicine")
 *   LED         -> GPIO 2 (onboard)
 *
 * FIRST RUN
 *   1. Edit include/config.h: set STA_SSID / STA_PASSWORD (or switch to
 *      MODE_AP for a standalone hotspot).
 *   2. Flash, open Serial Monitor at 115200.
 *   3. The IP address is printed. Open it in a phone browser.
 */

#include <Arduino.h>

#include "config.h"
#include "schedule.h"
#include "reminder.h"
#include "web.h"

// ---------------------------------------------------------------------------
// Boot banner
// ---------------------------------------------------------------------------

static void printBanner() {
  Serial.println();
  Serial.println(F("=============================================="));
  Serial.printf( "  %s  v%s\n", FITALIS_NAME, FITALIS_VERSION);
  Serial.println(F("  Smart Medicine Reminder"));
  Serial.println(F("=============================================="));
  Serial.printf( "  Device ID : %s\n", FITALIS_DEVICE_ID);
  Serial.printf( "  Buzzer    : GPIO %d\n", PIN_BUZZER);
  Serial.printf( "  Button    : GPIO %d\n", PIN_BUTTON_ACK);
  Serial.printf( "  WiFi mode : %s\n",
                 (WIFI_MODE == MODE_AP) ? "AP (standalone hotspot)" : "STATION (join router)");
  Serial.println(F("----------------------------------------------"));
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(300);
  printBanner();

  // 1. Load persisted schedules first — the reminder must work even if
  //    networking fails completely.
  scheduleInit();
  reminderInit();
  LOGF("[main] %d schedule(s) in flash\n", scheduleCount());

  // 2. Bring up networking + NTP. Never blocks the reminder for long:
  //    worst case it falls back to AP mode and the clock is set from
  //    the dashboard.
  bool timeOk = webConnectAndSyncTime();

  if (!timeOk && WIFI_MODE == MODE_STATION) {
    LOGLN("[main] running without synced clock — set time from the dashboard");
  }

  // 3. Start the dashboard.
  webInit();

  Serial.println(F("----------------------------------------------"));
  Serial.printf( "  Dashboard : http://%s/\n", webLocalIp().c_str());
  Serial.printf( "  Clock     : %s %s\n", dateNowString().c_str(), timeNowString().c_str());
  Serial.printf( "  Free RAM  : %u bytes\n", (unsigned)ESP.getFreeHeap());
  Serial.println(F("=============================================="));
  Serial.println();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------

void loop() {
  // Order matters: the alarm check runs every iteration so a due dose
  // is never missed while the web server is busy handling a request.
  reminderLoop();
  webLoop();
  delay(2);
}

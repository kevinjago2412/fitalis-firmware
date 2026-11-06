/*
 * FITALIS — Smart Medicine Reminder
 * Configuration
 * ---------------------------------------------------------------
 * Board  : ESP32 DevKit V1 (ESP32-WROOM-32)
 * Target : Arduino IDE 2.x  /  PlatformIO (env:esp32dev)
 *
 * Change values here only. Nothing else in the project hardcodes pins.
 */

#ifndef FITALIS_CONFIG_H
#define FITALIS_CONFIG_H

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------
#define FITALIS_NAME          "FITALIS"
#define FITALIS_VERSION       "0.1.0"
#define FITALIS_DEVICE_ID     "fitalis-01"

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
// Two modes:
//   WIFI_MODE_AP      -> ESP32 becomes its own access point (no router needed).
//                        Dashboard at http://192.168.4.1
//                        BUT: no internet => NTP time sync unavailable.
//   WIFI_MODE_STATION -> ESP32 joins your home WiFi/router.
//                        Gets NTP time. Dashboard at the IP printed on Serial.
#define WIFI_MODE_AP          0
#define WIFI_MODE_STATION     1

#define WIFI_MODE             WIFI_MODE_STATION

#define AP_SSID               "FITALIS"
#define AP_PASSWORD           "fitalis123"     // min 8 chars; "" for open AP

#define STA_SSID              "GANTI_DENGAN_WIFI_KAMU"
#define STA_PASSWORD          "GANTI_DENGAN_PASSWORD"

// Try to reconnect to WiFi for this long before giving up on NTP.
#define WIFI_CONNECT_TIMEOUT_MS   20000UL

// ---------------------------------------------------------------------------
// Pins — ESP32 DevKit V1
// ---------------------------------------------------------------------------
#define PIN_BUZZER            25    // active buzzer (+)  — (--) to GND
#define PIN_LED_STATUS        2     // onboard LED, blinks while alarm rings
#define PIN_BUTTON_ACK        27    // push button to GND = "I took my medicine"

// Reserved for a later phase (not used in v0.1.0):
// #define PIN_TFT_CS         5
// #define PIN_TFT_DC         4
// #define PIN_TFT_RST        15
// #define PIN_TFT_MOSI       23
// #define PIN_TFT_SCLK       18
// #define PIN_IR_SENSOR      34

// Active buzzer: HIGH = sounding. Set to 0 if you use a passive buzzer.
#define BUZZER_ACTIVE_HIGH    1

// Button uses INPUT_PULLUP, so pressed == LOW
#define BUTTON_PRESSED_LEVEL  LOW

// ---------------------------------------------------------------------------
// Schedule limits
// ---------------------------------------------------------------------------
#define MAX_SCHEDULES         8      // concurrent medicine schedules
#define MAX_NAME_LEN          32
#define MAX_DOSE_LEN          16     // e.g. "1 tablet", "5 ml"
#define NTP_SERVER_1          "pool.ntp.org"
#define NTP_SERVER_2          "time.google.com"
#define TZ_OFFSET_HOURS       7      // WIB = UTC+7. Change for your timezone.

// ---------------------------------------------------------------------------
// Alarm behaviour
// ---------------------------------------------------------------------------
#define ALARM_DURATION_MS     60000UL   // how long the alarm keeps ringing
#define ALARM_BEEP_ON_MS      400UL     // buzzer on/off rhythm
#define ALARM_BEEP_OFF_MS     600UL
#define ALARM_LED_BLINK_MS    200UL
#define ALARM_REPEAT_MINS     5         // re-ring this many minutes after being ignored
#define ALARM_MAX_REPEATS     2         // then give up until the next day

// Only fire an alarm when the current minute is within this many minutes of
// the scheduled time. Guards against a reboot at 23:59 firing a 08:00 alarm.
#define ALARM_WINDOW_MINS     2

// ---------------------------------------------------------------------------
// Notification (feature 4 — optional, off by default)
// ---------------------------------------------------------------------------
// Fires an HTTP POST to a URL of your choice exactly once when an alarm
// starts. Works with Telegram Bot API, ntfy.sh, or your own server.
// The reminder itself works fine with this disabled.
#define NOTIFY_ENABLED        0
#define NOTIFY_MAX_FAILURES   3        // stop trying after N consecutive failures
#define NOTIFY_TIMEOUT_MS     8000

// Example — ntfy.sh (public, no account needed):
//   NOTIFY_URL "http://ntfy.sh/fitalis-obat-kevin-9x2"
// Example — Telegram:
//   NOTIFY_URL "https://api.telegram.org/bot<TOKEN>/sendMessage?chat_id=<ID>&text={msg}"
// {msg} is replaced with the URL-encoded message. If the URL has no {msg},
// the message is sent as a JSON body: {"text": "..."}
#define NOTIFY_URL            "http://ntfy.sh/fitalis-obat-ganti-ini"

// ---------------------------------------------------------------------------
// Web server
// ---------------------------------------------------------------------------
#define WEB_PORT              80

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------
#define FITALIS_DEBUG         1

#if FITALIS_DEBUG
  #define LOGF(...)   Serial.printf(__VA_ARGS__)
  #define LOGLN(x)    Serial.println(x)
#else
  #define LOGF(...)   do {} while (0)
  #define LOGLN(x)    do {} while (0)
#endif

#endif // FITALIS_CONFIG_H

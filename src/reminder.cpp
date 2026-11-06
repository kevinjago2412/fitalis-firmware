#include "reminder.h"
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>

static ActiveAlarm    s_alarm;
static uint32_t       s_lastCheck = 0;

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------

bool timeIsSynced() {
  // Anything past 2021-01-01 means NTP (or the browser) set the clock.
  return time(nullptr) > 1609459200;
}

struct tm timeNowTm() {
  struct tm t;
  time_t now = time(nullptr);
  localtime_r(&now, &t);
  return t;
}

String timeNowString() {
  char buf[16];
  if (!timeIsSynced()) return String("--:--:--");
  struct tm t = timeNowTm();
  strftime(buf, sizeof(buf), "%H:%M:%S", &t);
  return String(buf);
}

String dateNowString() {
  char buf[16];
  if (!timeIsSynced()) return String("---- -- --");
  struct tm t = timeNowTm();
  strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
  return String(buf);
}

uint32_t todayDateKey() {
  if (!timeIsSynced()) return 0;
  struct tm t = timeNowTm();
  return (uint32_t)(t.tm_year + 1900) * 10000UL
       + (uint32_t)(t.tm_mon + 1) * 100UL
       + (uint32_t)t.tm_mday;
}

// Called by the dashboard when NTP is unavailable (AP mode).
// Format: "2026-11-06T14:05"
bool timeSetFromClient(const String& isoLocal) {
  if (isoLocal.length() < 16) return false;

  int year  = isoLocal.substring(0, 4).toInt();
  int month = isoLocal.substring(5, 7).toInt();
  int day   = isoLocal.substring(8, 10).toInt();
  int hour  = isoLocal.substring(11, 13).toInt();
  int mins  = isoLocal.substring(14, 16).toInt();

  if (year < 2021 || month < 1 || month > 12 || day < 1 || day > 31) return false;
  if (hour < 0 || hour > 23 || mins < 0 || mins > 59) return false;

  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon  = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min  = mins;
  t.tm_sec  = 0;
  t.tm_isdst = 0;

  time_t epoch = mktime(&t);
  if (epoch < 1609459200) return false;

  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  LOGF("[reminder] clock set manually -> %04d-%02d-%02d %02d:%02d\n",
       year, month, day, hour, mins);
  return true;
}

// ---------------------------------------------------------------------------
// Hardware
// ---------------------------------------------------------------------------

static void buzzerOn()  { digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_HIGH ? HIGH : LOW); }
static void buzzerOff() { digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_HIGH ? LOW : HIGH); }

// ---------------------------------------------------------------------------
// Notification (optional — feature 4)
// ---------------------------------------------------------------------------

#if NOTIFY_ENABLED

static uint8_t s_notifyFailures = 0;

static String urlEncode(const String& in) {
  String out;
  out.reserve(in.length() * 3);
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else if (c == ' ') {
      out += '+';
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

static void sendNotification(const Schedule& s) {
  if (WiFi.status() != WL_CONNECTED) {
    LOGLN("[notify] skipped: no WiFi");
    return;
  }
  if (s_notifyFailures >= NOTIFY_MAX_FAILURES) {
    LOGLN("[notify] skipped: too many previous failures");
    return;
  }

  String message = String(FITALIS_NAME) + ": waktunya minum " + s.name
                 + " (" + s.dose + ") jam " + timeNowString().substring(0, 5);

  String url = String(NOTIFY_URL);
  bool hasPlaceholder = url.indexOf("{msg}") >= 0;
  if (hasPlaceholder) {
    url.replace("{msg}", urlEncode(message));
  }

  HTTPClient http;
  http.setConnectTimeout(NOTIFY_TIMEOUT_MS);
  http.setTimeout(NOTIFY_TIMEOUT_MS);

  if (!http.begin(url)) {
    LOGLN("[notify] begin failed");
    s_notifyFailures++;
    return;
  }

  int code;
  if (hasPlaceholder) {
    code = http.GET();
  } else {
    http.addHeader("Content-Type", "application/json");
    String body = "{\"text\":\"" + message + "\"}";
    code = http.POST(body);
  }
  http.end();

  if (code >= 200 && code < 300) {
    LOGF("[notify] sent (HTTP %d)\n", code);
    s_notifyFailures = 0;
  } else {
    LOGF("[notify] failed (HTTP %d)\n", code);
    s_notifyFailures++;
  }
}

#endif // NOTIFY_ENABLED

// ---------------------------------------------------------------------------
// Alarm control
// ---------------------------------------------------------------------------

static void startAlarm(int index) {
  Schedule* s = scheduleGet(index);
  if (!s) return;

  s_alarm.active        = true;
  s_alarm.scheduleIndex = index;
  s_alarm.startedAt     = millis();
  s_alarm.repeatCount   = 0;
  s_alarm.nextRepeatAt  = millis() + (uint32_t)ALARM_REPEAT_MINS * 60000UL;

  LOGF("[reminder] ALARM -> %s (%s) %02d:%02d\n",
       s->name, s->dose, s->hour, s->minute);

#if NOTIFY_ENABLED
  sendNotification(*s);
#endif
}

static void stopAlarm() {
  s_alarm.active = false;
  s_alarm.scheduleIndex = -1;
  buzzerOff();
  digitalWrite(PIN_LED_STATUS, LOW);
}

void reminderInit() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_BUTTON_ACK, INPUT_PULLUP);
  buzzerOff();
  digitalWrite(PIN_LED_STATUS, LOW);

  s_alarm.active        = false;
  s_alarm.scheduleIndex = -1;
  s_alarm.repeatCount   = 0;
  s_alarm.startedAt     = 0;
  s_alarm.nextRepeatAt  = 0;
}

// ---------------------------------------------------------------------------
// Ringing pattern — non-blocking
// ---------------------------------------------------------------------------

static void driveBuzzerPattern() {
  uint32_t elapsed = millis() - s_alarm.startedAt;
  uint32_t cycle   = ALARM_BEEP_ON_MS + ALARM_BEEP_OFF_MS;
  bool on = (elapsed % cycle) < ALARM_BEEP_ON_MS;

  if (BUZZER_ACTIVE_HIGH) {
    digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  } else {
    digitalWrite(PIN_BUZZER, on ? LOW : HIGH);
  }

  bool ledOn = ((millis() / ALARM_LED_BLINK_MS) % 2) == 0;
  digitalWrite(PIN_LED_STATUS, ledOn ? HIGH : LOW);
}

// ---------------------------------------------------------------------------
// Button — debounced
// ---------------------------------------------------------------------------

static bool readButtonPress() {
  static uint32_t lastPress = 0;
  static bool     lastState = false;

  bool pressed = (digitalRead(PIN_BUTTON_ACK) == BUTTON_PRESSED_LEVEL);
  bool edge = pressed && !lastState;
  lastState = pressed;

  if (edge && (millis() - lastPress > 400)) {
    lastPress = millis();
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Schedule matching
// ---------------------------------------------------------------------------

static bool dayMatches(uint8_t daysMask, int wday) {
  // struct tm: 0 = Sunday. Bit 0 of the mask is also Sunday.
  return (daysMask & (1 << wday)) != 0;
}

static void checkSchedules() {
  if (!timeIsSynced()) return;

  struct tm t = timeNowTm();
  uint32_t dk = todayDateKey();

  for (int i = 0; i < scheduleCount(); i++) {
    Schedule* s = scheduleGet(i);
    if (!s || !s->enabled) continue;

    if (!dayMatches(s->days, t.tm_wday))   continue;
    if (s->takenDateKey == dk)             continue;   // already taken today

    // Is the current minute the scheduled minute?
    if (t.tm_hour != s->hour || t.tm_min != s->minute) continue;

    // Window guard — don't fire on a reboot hours late.
    // (Exact minute match above already handles this; the window is here
    //  so a schedule set for :00 still fires if the loop lands at :01.)
    if (t.tm_sec > 59) continue;

    if (s_alarm.active && s_alarm.scheduleIndex == i) continue;  // already ringing

    startAlarm(i);
    break;   // one alarm at a time
  }
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void reminderLoop() {
  // 1. Handle an active alarm
  if (s_alarm.active) {
    driveBuzzerPattern();

    if (readButtonPress()) {
      LOGLN("[reminder] acknowledged by button");
      reminderAcknowledge();
      return;
    }

    uint32_t elapsed = millis() - s_alarm.startedAt;

    // Auto-stop after ALARM_DURATION_MS
    if (elapsed >= ALARM_DURATION_MS) {
      buzzerOff();
      digitalWrite(PIN_LED_STATUS, LOW);

      if (s_alarm.repeatCount < ALARM_MAX_REPEATS) {
        // Wait, then ring again
        if ((int32_t)(millis() - s_alarm.nextRepeatAt) >= 0) {
          s_alarm.repeatCount++;
          s_alarm.startedAt = millis();
          s_alarm.nextRepeatAt = millis() + (uint32_t)ALARM_REPEAT_MINS * 60000UL;
          LOGF("[reminder] repeat #%d\n", s_alarm.repeatCount);
        }
      } else {
        LOGLN("[reminder] giving up after max repeats");
        stopAlarm();
      }
    }
    return;
  }

  // 2. Poll schedules once per second
  if (millis() - s_lastCheck < 1000) return;
  s_lastCheck = millis();

  checkSchedules();
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

bool reminderAcknowledge() {
  if (!s_alarm.active) return false;

  Schedule* s = scheduleGet(s_alarm.scheduleIndex);
  if (s) {
    s->takenDateKey = todayDateKey();
    scheduleSave();
    LOGF("[reminder] marked taken: %s @ %s\n", s->name, timeNowString().c_str());
  }
  stopAlarm();
  return true;
}

bool reminderDismiss() {
  if (!s_alarm.active) return false;
  LOGLN("[reminder] dismissed without marking taken");
  stopAlarm();
  return true;
}

bool reminderSnooze() {
  if (!s_alarm.active) return false;

  s_alarm.startedAt    = millis();
  s_alarm.nextRepeatAt = millis() + (uint32_t)ALARM_REPEAT_MINS * 60000UL;
  s_alarm.repeatCount  = 0;
  LOGF("[reminder] snoozed %d min\n", ALARM_REPEAT_MINS);
  return true;
}

// ---------------------------------------------------------------------------
// Status for the dashboard
// ---------------------------------------------------------------------------

ReminderState reminderGetState() {
  return s_alarm.active ? REMINDER_RINGING : REMINDER_IDLE;
}

int reminderActiveIndex() {
  return s_alarm.active ? s_alarm.scheduleIndex : -1;
}

String reminderStatusJson() {
  String out;
  out.reserve(320);

  out += "{\"state\":\"";
  out += s_alarm.active ? "ringing" : "idle";
  out += "\",\"activeIndex\":";
  out += s_alarm.active ? s_alarm.scheduleIndex : -1;

  out += ",\"time\":\"";   out += timeNowString();  out += "\"";
  out += ",\"date\":\"";   out += dateNowString();  out += "\"";
  out += ",\"synced\":";   out += timeIsSynced() ? "true" : "false";
  out += ",\"uptime\":";   out += (millis() / 1000);
  out += ",\"heap\":";     out += ESP.getFreeHeap();

  if (s_alarm.active) {
    Schedule* s = scheduleGet(s_alarm.scheduleIndex);
    if (s) {
      out += ",\"activeName\":\"";  out += s->name;  out += "\"";
      out += ",\"activeDose\":\"";  out += s->dose;  out += "\"";
    }
    out += ",\"elapsed\":"; out += (millis() - s_alarm.startedAt);
    out += ",\"repeats\":"; out += s_alarm.repeatCount;
  }

  out += "}";
  return out;
}

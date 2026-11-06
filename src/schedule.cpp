#include "schedule.h"
#include <Preferences.h>

/*
 * Schedules are stored in NVS (non-volatile storage) via the Preferences
 * library that ships with the ESP32 Arduino core.
 *
 * Layout: one blob holding the whole array. Simple, atomic, and survives
 * power cuts. MAX_SCHEDULES is small so size is not a concern.
 */

static const char* NVS_NAMESPACE = "fitalis";
static const char* NVS_KEY       = "sched";

static Schedule s_schedules[MAX_SCHEDULES];
static int      s_count = 0;

static Preferences s_prefs;

// ---------------------------------------------------------------------------

void scheduleInit() {
  s_prefs.begin(NVS_NAMESPACE, false);

  size_t len = s_prefs.getBytesLength(NVS_KEY);
  if (len == sizeof(s_schedules)) {
    s_prefs.getBytes(NVS_KEY, s_schedules, len);
    s_count = 0;
    // Count how many leading entries look initialized (name non-empty).
    for (int i = 0; i < MAX_SCHEDULES; i++) {
      if (s_schedules[i].name[0] != '\0') s_count = i + 1;
    }
    LOGF("[schedule] loaded %d schedule(s) from NVS\n", s_count);
  } else {
    s_count = 0;
    memset(s_schedules, 0, sizeof(s_schedules));
    LOGLN("[schedule] no saved schedules, starting empty");
  }
}

int scheduleCount() {
  return s_count;
}

Schedule* scheduleGet(int index) {
  if (index < 0 || index >= s_count) return nullptr;
  return &s_schedules[index];
}

static void copyStr(char* dst, size_t dstLen, const char* src) {
  if (!src) { dst[0] = '\0'; return; }
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

int scheduleAdd(const char* name, const char* dose,
                uint8_t hour, uint8_t minute, uint8_t days) {
  if (s_count >= MAX_SCHEDULES) {
    LOGLN("[schedule] add failed: table full");
    return -1;
  }
  if (hour > 23 || minute > 59 || days == 0) {
    LOGLN("[schedule] add failed: invalid time/days");
    return -1;
  }

  Schedule& s = s_schedules[s_count];
  s.enabled      = true;
  s.hour         = hour;
  s.minute       = minute;
  s.days         = days;
  s.takenDateKey = 0;
  copyStr(s.name, MAX_NAME_LEN, (name && name[0]) ? name : "Obat");
  copyStr(s.dose, MAX_DOSE_LEN, (dose && dose[0]) ? dose : "-");

  s_count++;
  scheduleSave();
  LOGF("[schedule] added #%d %s at %02d:%02d\n", s_count - 1, s.name, hour, minute);
  return s_count - 1;
}

bool scheduleUpdate(int index, const char* name, const char* dose,
                    uint8_t hour, uint8_t minute, uint8_t days, bool enabled) {
  if (index < 0 || index >= s_count) return false;
  if (hour > 23 || minute > 59 || days == 0) return false;

  Schedule& s = s_schedules[index];
  // If the time changes, the previous "taken" marker no longer applies.
  if (s.hour != hour || s.minute != minute) {
    s.takenDateKey = 0;
  }
  s.hour    = hour;
  s.minute  = minute;
  s.days    = days;
  s.enabled = enabled;
  copyStr(s.name, MAX_NAME_LEN, name);
  copyStr(s.dose, MAX_DOSE_LEN, dose);

  scheduleSave();
  return true;
}

bool scheduleDelete(int index) {
  if (index < 0 || index >= s_count) return false;

  // Shift the tail down so indices stay contiguous.
  for (int i = index; i < s_count - 1; i++) {
    s_schedules[i] = s_schedules[i + 1];
  }
  s_count--;
  memset(&s_schedules[s_count], 0, sizeof(Schedule));
  scheduleSave();
  LOGF("[schedule] deleted #%d, %d left\n", index, s_count);
  return true;
}

void scheduleSave() {
  s_prefs.putBytes(NVS_KEY, s_schedules, sizeof(s_schedules));
  LOGLN("[schedule] saved to NVS");
}

void scheduleClearTakenFlags() {
  for (int i = 0; i < s_count; i++) {
    s_schedules[i].takenDateKey = 0;
  }
  scheduleSave();
}

// ---------------------------------------------------------------------------
// JSON output — hand-built to avoid a JSON library dependency.
// ---------------------------------------------------------------------------

static void appendEscaped(String& out, const char* s) {
  for (const char* p = s; *p; ++p) {
    char c = *p;
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c == '\n' || c == '\r') {
      out += ' ';
    } else {
      out += c;
    }
  }
}

String scheduleToJson() {
  String out;
  out.reserve(256 + s_count * 160);
  out += "{\"count\":";
  out += s_count;
  out += ",\"max\":";
  out += MAX_SCHEDULES;
  out += ",\"schedules\":[";

  for (int i = 0; i < s_count; i++) {
    Schedule& s = s_schedules[i];
    if (i) out += ',';
    out += "{\"index\":";
    out += i;
    out += ",\"name\":\"";
    appendEscaped(out, s.name);
    out += "\",\"dose\":\"";
    appendEscaped(out, s.dose);
    out += "\",\"hour\":";
    out += s.hour;
    out += ",\"minute\":";
    out += s.minute;
    out += ",\"days\":";
    out += s.days;
    out += ",\"enabled\":";
    out += s.enabled ? "true" : "false";
    out += ",\"taken\":";
    out += (s.takenDateKey != 0) ? "true" : "false";
    out += '}';
  }

  out += "]}";
  return out;
}

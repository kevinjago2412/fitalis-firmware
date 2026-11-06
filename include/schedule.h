/*
 * FITALIS — Schedule storage (NVS / Preferences)
 * ---------------------------------------------------------------
 * Schedules persist in flash, so they survive power loss and WiFi
 * outages. The reminder keeps working with no network at all.
 */

#ifndef FITALIS_SCHEDULE_H
#define FITALIS_SCHEDULE_H

#include <Arduino.h>
#include "config.h"

#define DAY_SUN  0x01
#define DAY_MON  0x02
#define DAY_TUE  0x04
#define DAY_WED  0x08
#define DAY_THU  0x10
#define DAY_FRI  0x20
#define DAY_SAT  0x40
#define DAY_ALL  0x7F

struct Schedule {
  bool     enabled;
  uint8_t  hour;        // 0-23
  uint8_t  minute;      // 0-59
  uint8_t  days;        // bitmask, bit 0 = Sunday
  char     name[MAX_NAME_LEN];
  char     dose[MAX_DOSE_LEN];
  uint32_t takenDateKey;  // YYYYMMDD of last acknowledgment; 0 = never
};

void scheduleInit();
int  scheduleCount();
Schedule* scheduleGet(int index);
int  scheduleAdd(const char* name, const char* dose,
                 uint8_t hour, uint8_t minute, uint8_t days);
bool scheduleUpdate(int index, const char* name, const char* dose,
                    uint8_t hour, uint8_t minute, uint8_t days, bool enabled);
bool scheduleDelete(int index);
void scheduleSave();
void scheduleClearTakenFlags();

// Returns a compact JSON array (no pretty printing — the dashboard parses it).
String scheduleToJson();

#endif // FITALIS_SCHEDULE_H

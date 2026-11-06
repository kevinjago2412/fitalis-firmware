/*
 * FITALIS — Reminder engine
 * ---------------------------------------------------------------
 * Checks every schedule once per second and rings the buzzer when a
 * dose is due. Acknowledging (button or dashboard) stops the alarm
 * and marks that dose as taken for the day.
 */

#ifndef FITALIS_REMINDER_H
#define FITALIS_REMINDER_H

#include <Arduino.h>
#include "schedule.h"

enum ReminderState {
  REMINDER_IDLE = 0,
  REMINDER_RINGING,
  REMINDER_SNOOZED
};

struct ActiveAlarm {
  bool     active;
  int      scheduleIndex;
  uint32_t startedAt;      // millis()
  uint32_t nextRepeatAt;   // millis()
  uint8_t  repeatCount;
};

void reminderInit();
void reminderLoop();

// Acknowledge the currently ringing alarm ("I took my medicine").
bool reminderAcknowledge();
// Skip this occurrence without marking it taken.
bool reminderDismiss();
// Ring again after ALARM_REPEAT_MINS.
bool reminderSnooze();

// --- Status for the web UI ---------------------------------------------
ReminderState reminderGetState();
int           reminderActiveIndex();
String        reminderStatusJson();

// --- Time helpers -------------------------------------------------------
bool     timeIsSynced();
String   timeNowString();       // "14:05:33"
String   dateNowString();       // "2026-11-06"
uint32_t todayDateKey();        // 20261106, 0 if time not synced
bool     timeSetFromClient(const String& isoLocal);  // "2026-11-06T14:05"

#endif // FITALIS_REMINDER_H

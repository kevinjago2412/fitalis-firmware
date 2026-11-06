/*
 * FITALIS — Network + Web Dashboard
 * ---------------------------------------------------------------
 * Serves the dashboard and the JSON API the page talks to.
 * Runs on port 80 so you just open http://192.168.4.1 (AP mode) or
 * the IP shown on Serial (station mode).
 */

#ifndef FITALIS_WEB_H
#define FITALIS_WEB_H

#include <Arduino.h>

void  webInit();          // start WiFi + NTP (non-blocking parts)
void  webLoop();          // handle clients
String webLocalIp();      // current IP as a string
bool  webIsConnected();

// Called once at boot; returns true if WiFi connected and time synced.
bool  webConnectAndSyncTime();

#endif // FITALIS_WEB_H

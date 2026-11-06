#!/usr/bin/env python3
"""Bundle the FITALIS multi-file firmware into a single Arduino .ino sketch.

Run from anywhere:
    python3 build_ino.py

Output:
    /sdcard/Projects/fitalis-firmware/FITALIS/FITALIS.ino
    /root/fitalis-ino-test/src/main.cpp   (for CI verification)
"""
from __future__ import annotations

import os
import re

SRC = "/root/fitalis-firmware"
OUT_DIR = "/sdcard/Projects/fitalis-firmware/FITALIS"
CI_PROJ = "/root/fitalis-ino-test"


def read(rel: str) -> str:
    with open(os.path.join(SRC, rel), encoding="utf-8") as f:
        return f.read()


def strip_local_includes(text: str) -> str:
    """Drop #include "config.h" etc — everything ends up in one file."""
    out = []
    for line in text.splitlines():
        s = line.strip()
        if s.startswith('#include "') and s.rstrip('"').endswith(
            ("config.h", "schedule.h", "reminder.h", "web.h")
        ):
            continue
        out.append(line)
    return "\n".join(out).strip()


def strip_guards(text: str) -> str:
    """Remove the #ifndef/#define/#endif include guard from a header."""
    out, inside = [], False
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#ifndef FITALIS_") and s.endswith("_H"):
            inside = True
            continue
        if inside and s.startswith("#define FITALIS_") and s.endswith("_H"):
            continue
        if inside and s.startswith("#endif") and "FITALIS_" in s:
            inside = False
            continue
        out.append(line)
    return "\n".join(out).strip()


HEADER = """/*******************************************************************************
 * FITALIS — Smart Medicine Reminder
 * Pengingat Minum Obat berbasis ESP32 DevKit V1
 * -----------------------------------------------------------------------------
 * Versi    : {version}
 * Board    : ESP32 DevKit V1  (Arduino IDE: "ESP32 Dev Module")
 * Serial   : 115200 baud
 *
 * CARA PAKAI:
 *   1. Edit "BAGIAN 1 — KONFIGURASI" di bawah: isi STA_SSID dan STA_PASSWORD
 *      dengan WiFi kamu (atau set WIFI_MODE ke MODE_AP untuk mode hotspot).
 *   2. Upload ke ESP32 lewat Arduino IDE.
 *   3. Buka Serial Monitor (115200). IP address akan muncul di situ.
 *   4. Buka IP itu di browser HP untuk setting jadwal minum obat.
 *
 * HARDWARE:
 *   Buzzer aktif (+)  -> GPIO 25      Buzzer (-) -> GND
 *   Tombol konfirmasi -> GPIO 27      kaki lain  -> GND
 *   LED indikator     -> GPIO 2 (onboard)
 *
 * File ini adalah gabungan seluruh source code jadi SATU sketch, supaya bisa
 * langsung dibuka di Arduino IDE tanpa file tambahan.
 ******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>

/* =========================================================================
 * BAGIAN 1 — KONFIGURASI  (EDIT DI SINI)
 * ========================================================================= */

{config_body}

/* =========================================================================
 * BAGIAN 2 — HEADER: SCHEDULE
 * ========================================================================= */

{schedule_h}

/* =========================================================================
 * BAGIAN 3 — HEADER: REMINDER
 * ========================================================================= */

{reminder_h}

/* =========================================================================
 * BAGIAN 4 — HEADER: WEB
 * ========================================================================= */

{web_h}

/* =========================================================================
 * BAGIAN 5 — SCHEDULE (penyimpanan jadwal di flash)
 * ========================================================================= */

{schedule_cpp}

/* =========================================================================
 * BAGIAN 6 — REMINDER (mesin alarm + jam)
 * ========================================================================= */

{reminder_cpp}

/* =========================================================================
 * BAGIAN 7 — WEB SERVER + DASHBOARD
 * ========================================================================= */

{web_cpp}

/* =========================================================================
 * BAGIAN 8 — MAIN (setup + loop)
 * ========================================================================= */

{main_cpp}
"""


def build() -> str:
    version = re.search(r'#define FITALIS_VERSION\s+"([^"]+)"', read("include/config.h"))
    version = version.group(1) if version else "0.0.0"

    ino = HEADER.format(
        version=version,
        config_body=strip_guards(read("include/config.h")),
        schedule_h=strip_guards(read("include/schedule.h")),
        reminder_h=strip_guards(read("include/reminder.h")),
        web_h=strip_guards(read("include/web.h")),
        schedule_cpp=strip_local_includes(read("src/schedule.cpp")),
        reminder_cpp=strip_local_includes(read("src/reminder.cpp")),
        web_cpp=strip_local_includes(read("src/web.cpp")),
        main_cpp=strip_local_includes(read("src/main.cpp")),
    )

    # Belt and braces: no local includes may survive.
    ino = re.sub(r'^#include "(config|schedule|reminder|web)\.h"\s*$', "", ino, flags=re.M)
    ino = re.sub(r"\n{4,}", "\n\n\n", ino)
    return ino


def main() -> int:
    ino = build()

    os.makedirs(OUT_DIR, exist_ok=True)
    ino_path = os.path.join(OUT_DIR, "FITALIS.ino")
    with open(ino_path, "w", encoding="utf-8") as f:
        f.write(ino)

    os.makedirs(os.path.join(CI_PROJ, "src"), exist_ok=True)
    with open(os.path.join(CI_PROJ, "src", "main.cpp"), "w", encoding="utf-8") as f:
        f.write(ino)

    print(f"built {ino_path}")
    print(f"  size : {len(ino.encode()):,} bytes")
    print(f"  lines: {ino.count(chr(10)) + 1}")
    print(f"  setup(): {ino.count('void setup()')}  loop(): {ino.count('void loop()')}")
    leftover = re.findall(r'#include\s+"[^"]+"', ino)
    print(f"  leftover local includes: {leftover or 'none'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#ifndef BOAT_CONFIG_H
#define BOAT_CONFIG_H

#include <Arduino.h>

// Optional local Wi-Fi credentials live in src/local_wifi_config.h.
// That file is ignored by git so personal hotspot passwords stay local.
#if __has_include("local_wifi_config.h")
#include "local_wifi_config.h"
#endif

#ifndef BOAT_STA_ENABLED
#define BOAT_STA_ENABLED 0
#endif

#ifndef BOAT_STA_SSID
#define BOAT_STA_SSID ""
#endif

#ifndef BOAT_STA_PASSWORD
#define BOAT_STA_PASSWORD ""
#endif

#ifndef BOAT_STA_CONNECT_TIMEOUT_MS
#define BOAT_STA_CONNECT_TIMEOUT_MS 10000UL
#endif

#ifndef BOAT_STA_RECONNECT_INTERVAL_MS
#define BOAT_STA_RECONNECT_INTERVAL_MS 15000UL
#endif

// 0 keeps the current auto-channel behavior. Set to 1, 6, or 11 to force a
// fixed 2.4 GHz Wi-Fi channel after field testing.
static constexpr uint8_t BOAT_WIFI_CHANNEL = 0;

// The web page repeats the current A/B throttle values at this interval.
static constexpr unsigned long DRIVE_SEND_INTERVAL_MS = 150;

// If the active controller stops sending drive packets for this long, stop.
static constexpr unsigned long CONTROL_TIMEOUT_MS = 700;

// Small joystick/slider jitter below this value is treated as stop.
static constexpr int DRIVE_DEADZONE = 15;

// Nonzero throttle maps into this duty range. The lower bound is high enough
// to overcome the propeller load while still preserving slider control.
static constexpr int DRIVE_MIN_DUTY = 210;
static constexpr int DRIVE_MAX_DUTY = 255;

// Full-power kick used only when starting from stop or changing direction.
static constexpr int DRIVE_START_KICK_DUTY = 255;
static constexpr int DRIVE_START_KICK_MS = 150;

#endif // BOAT_CONFIG_H

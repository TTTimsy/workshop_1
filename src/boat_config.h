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
static constexpr int DRIVE_DEADZONE = 18;

// Racing profile: nonzero throttle maps into this useful motor range.
// The lower bound stays high because low PWM cannot reliably spin the prop.
static constexpr int DRIVE_MIN_DUTY = 220;
static constexpr int DRIVE_MID_DUTY = 238;
static constexpr int DRIVE_MAX_DUTY = 255;

// DriveSystem update period and output slew limits.
static constexpr unsigned long DRIVE_UPDATE_INTERVAL_MS = 20;
static constexpr int DRIVE_ACCEL_STEP = 12;
static constexpr int DRIVE_DECEL_STEP = 22;

// Full-power kick used only when starting from stop or after direction change.
static constexpr int DRIVE_START_KICK_DUTY = 255;
static constexpr int DRIVE_START_KICK_MS = 120;

// High-speed turning guard. Low speed keeps flexible tank steering; high speed
// limits left/right thrust difference so the boat does not snap-turn.
static constexpr int DRIVE_SPIN_INPUT_LIMIT = 220;
static constexpr int DRIVE_HIGH_SPEED_1 = 180;
static constexpr int DRIVE_HIGH_SPEED_2 = 220;
static constexpr int DRIVE_DIFF_LIMIT_1 = 90;
static constexpr int DRIVE_DIFF_LIMIT_2 = 65;

// Direction reversal guard: brake briefly before changing motor direction.
static constexpr unsigned long DRIVE_REVERSE_BRAKE_MS = 80;

#endif // BOAT_CONFIG_H

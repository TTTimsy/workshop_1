# ⛵ InnoX Boat — Student Lab

Welcome to the **InnoX Boat** embedded systems lab! You'll program an ESP32-C3
microcontroller to drive a twin-motor RC boat over Wi-Fi.

---

## Quick Start

1. **Connect the board** — Plug the Seeed XIAO ESP32C3 into USB.
2. **Open the project** — Open this folder in VS Code with PlatformIO installed.
3. **Build and upload:**
   ```bash
   pio run --target upload --upload-port COM9
   ```
   *(adjust `COM9` to whatever port your board appears on)*
4. **Open the Serial Monitor:**
   ```bash
   pio device monitor --port COM9 --baud 115200
   ```
5. **Connect to the boat** — Join the Wi-Fi network `InnoX-Boat-XXXX`
   and open **http://192.168.4.1** in your browser.
6. **Tap START** — You'll hear a chime from the motors, then you can use
   the throttle sliders to control them.

---

## Your Tasks

Open **`src/main.cpp`**. You'll find two functions marked with `✏️` that you
need to implement.

### Task 1 — `onStartMotors()` (the chime)

When the user taps the **START** button on the web page, this function runs.
Your job is to make the motors play a short melody using the `ChimePlayer`.

Some ideas:
- Three ascending notes (C4 → E4 → G4) on one motor
- A two-part harmony using both motors at once
- A simple tune you recognise

**Available tools:**

```cpp
// Single note (motorA plays C4 for 200 ms then stops)
chime.add(&motorA, NOTE_C4, 100, 200);

// Multi-note step (both motors play at the same time for 400 ms)
chime.startStep();
chime.addToStep(&motorA, NOTE_C4, 100);   // melody
chime.addToStep(&motorB, NOTE_E4, 80);    // harmony
chime.endStep(400);

// Start playing
chime.play();
```

**Pre-defined note constants** (see `src/chime.h` for the full list):
`NOTE_C4`, `NOTE_CS4` (C#), `NOTE_D4`, `NOTE_DS4` (D#), `NOTE_E4`,
`NOTE_F4`, `NOTE_FS4` (F#), `NOTE_G4`, `NOTE_GS4` (G#), `NOTE_A4`,
`NOTE_AS4` (A#), `NOTE_B4`, and the same pattern for octaves 1–7.

---

### Task 2 — `onMotorCommand()` (motor control)

Whenever you drag a throttle slider on the web page, this function is called
with:

| Parameter | Meaning |
|-----------|---------|
| `motor` | `'a'` or `'b'` — which motor to control |
| `speed` | `-255` (full backward) → `0` (stop) → `+255` (full forward) |

Your job is to translate the signed speed value into calls to the `Motor`
class:

| `Motor` method | What it does |
|---------------|-------------|
| `forward(0–255)` | Motor spins forward |  
| `backward(0–255)` | Motor spins backward |
| `stop()` | Motor stops |

The logic should be:

| speed | What to do |
|-------|-----------|
| **positive** (+1 to +255) | `forward(speed)` — forward at that speed |
| **negative** (-1 to -255) | `backward(-speed)` — backward (note: `backward()` expects a positive number!) |
| **zero** | `stop()` |

There's also an LED for each motor (`LED_A` on pin 20, `LED_B` on pin 21)
that you can light up while the motor is active.

---

## Project Structure

| File | What it does |
|------|-------------|
| `src/main.cpp` | **Your code lives here** — tasks 1 & 2 |
| `src/motor.h` | `Motor` class — `forward()`, `backward()`, `stop()`, `note()` |
| `src/chime.h` | `ChimePlayer` — non-blocking melody sequencer + note constants |
| `src/boat_controller.cpp` | Hidden — handles Wi-Fi, web server, WebSocket (don't touch) |
| `src/ap_manager.cpp` | Hidden — handles Wi-Fi access point setup (don't touch) |
| `src/html.h` | Hidden — the web page UI (don't touch) |

---

## Helpful Tips

- **Flash memory is limited** — the board has 1.3 MB. Keep your code efficient.
- **Non-blocking is better** — use `chime.playNote()` for sound, not
  `delay()`. The `ChimePlayer` handles timing in the background so the
  Wi-Fi / web server keeps running.
- **Test incrementally** — get the motor mapping working first, then
  experiment with the chime.
- **Check the Serial Monitor** — the boat prints status messages at
  115200 baud that will help you debug.

---

## Bonus: MIDI Easter Egg

You can play any MIDI file through the boat's motors!  See the commented
block at the end of `setup()` in `main.cpp` for instructions.

---

Happy boating! ⛵

# ⛵ InnoX Boat — Pre-Camp Activity

Welcome to the **InnoX Boat** embedded systems pre-camp activity! You'll program an ESP32-C3
microcontroller to drive a twin-motor RC boat over Wi-Fi.

---

## Quick Start

### 1. Connect the board
Plug the boat controller into your computer with a USB cable.

### 2. Find your board's COM port number

This is the #1 thing students get stuck on — here's how to find it:

**On Windows (most common):**
1. Press **Windows + X** → select **Device Manager**
2. Expand the **Ports (COM & LPT)** section
3. Look for something like **"USB Serial Device (COM5)"** or **"CP2102N USB to UART Bridge (COM7)"**
4. That **COM number** (e.g. `COM5`, `COM7`, `COM12`) is your board's port
5. If you don't see it: try a different USB cable (some are charge-only!), or install the CP2102N driver

**On macOS:**
The port will look like `/dev/cu.usbserial-XXXXXXXX` or `/dev/tty.usbserial-XXXXXXXX`.
Run `ls /dev/cu.usb*` or `ls /dev/tty.usb*` in Terminal to find it.

**On Linux:**
The port will look like `/dev/ttyUSB0` or `/dev/ttyACM0`. Run `ls /dev/ttyUSB* /dev/ttyACM*` to see it.

### 3. Open the project
Open this folder in VS Code with **PlatformIO** installed.

### 4. Build and upload (send your code to the board)

**Option A — From VS Code (recommended):**
- Click the **→ (arrow)** icon in the PlatformIO blue status bar at the bottom
- Or press Ctrl+Shift+P → type "PlatformIO: Upload" → press Enter
- PlatformIO usually auto-detects the port — if it asks, select your COM port

**Option B — From the terminal (if VS Code auto-detect fails):**
Replace `COM5` with **your** COM port number (from step 2):
```bash
pio run --target upload --upload-port COM5
```
*(On macOS/Linux, use `/dev/cu.usbserial-XXX` or `/dev/ttyUSB0` instead of `COM5`)*

**If the upload fails:**
- "Failed to connect" → Check your COM port number in Device Manager
- "Access denied" → Close any other program (like the Arduino IDE) that might be using that port
- Still stuck? → Try a different USB cable, or hold down the **B (RESET)** button on the board while uploading

### 5. Open the Serial Monitor (to see debug messages)

```bash
pio device monitor --port COM5 --baud 115200
```
*(Replace `COM5` with your COM port — same number from step 2)*

> **💡 Tip:** If you see garbled text (like `��␀␀␀`), the baud rate is wrong — make sure `--baud 115200` matches the `monitor_speed` in `platformio.ini`.

### 6. Connect to the boat

Join the Wi-Fi network **`InnoX-Boat-XXXX`** (the XXXX is unique to your board).
Open your browser and go to **http://192.168.4.1**

### 7. Tap START

You'll hear a chime from the motors, then you can use the throttle sliders to control them.

---

## Project Structure

| File | What to do with it |
|------|-------------------|
| `src/main.cpp` | **✏️  This is where you write all your code** |
| `src/motor.h` | Reference — the `Motor` class lives here (read but don't edit) |
| `src/chime.h` | Reference — the `ChimePlayer` and note definitions (read but don't edit) |
| everything else | Hidden "plumbing" — WiFi, web server, etc. Don't touch |

---

## Overview — How It All Connects

```
   Web Page                Boat (ESP32-C3)
┌─────────────┐         ┌──────────────────┐
│ Throttle    │  ──WS──→│ onMotorCommand() │──→ motorA.forward(200)
│ Slider A    │         │  ✏️ YOU WRITE THIS│
│             │         └──────────────────┘
│ Throttle    │         ┌──────────────────┐
│ Slider B    │  ──WS──→│ onMotorCommand() │──→ motorB.backward(150)
│             │         │  ✏️ YOU WRITE THIS│
│             │         └──────────────────┘
│ START btn   │  ──WS──→│ onStartMotors()  │──→ chime.add(&motorA, ...)
│             │         │  ✏️ YOU WRITE THIS│    chime.play()
└─────────────┘         └──────────────────┘
```

- **WS** = WebSocket (real-time connection between browser and boat)
- You write **two functions**. The system calls them automatically.
- **Don't worry** about WiFi, web servers, or WebSocket — that's all hidden.

---

# Task 1 — `onStartMotors()` (play a melody)

When the user taps the **START** button, the boat calls:

```cpp
void onStartMotors() {
  // ✏️  Your chime code goes here
}
```

Your job: make the motors play a short tune using the `ChimePlayer`.

### Step 1 — Play one note

The simplest thing you can do is play a single note on one motor:

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 300);   // motorA, C4, duty=80, 300ms
  chime.play();                             // start playing!
}
```

| Parameter | Meaning |
|-----------|---------|
| `&motorA` | Which motor to use (`&motorA` or `&motorB`) |
| `NOTE_C4` | Which note (frequency). See note table below. |
| `80` | Loudness (0 = silent, 255 = max). Start with 60–100. |
| `300` | Duration in milliseconds. 1000 = 1 second. |

Try it! Change the note, motor, or duration. Build + upload + tap START.

### Step 2 — Play multiple notes one after another

Just add more notes before `chime.play()`:

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();
}
```

This plays: **C4 → E4 → G4** (a C major arpeggio — sounds like a fanfare).

### Step 3 — Play two motors at once (harmony!)

To play both motors simultaneously, use steps:

```cpp
void onStartMotors() {
  chime.startStep();                               // begin a "chord"
  chime.addToStep(&motorA, NOTE_C4, 80);            // melody
  chime.addToStep(&motorB, NOTE_E4, 60);            // harmony (quieter)
  chime.endStep(400);                               // both play 400 ms

  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 80);
  chime.addToStep(&motorB, NOTE_C5, 60);
  chime.endStep(400);

  chime.play();
}
```

**Why `addToStep` instead of `add`?** — `chime.add()` queues one note at a time
(sequentially). `chime.startStep()` / `addToStep()` / `endStep()` lets you group
notes that start at the same time (simultaneously).

### Step 4 — Add LED blinking

You can blink the onboard LEDs while the chime plays:

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();

  // LED blinks (these run immediately because chime is non-blocking)
  digitalWrite(LED_A, HIGH);
  delay(150);
  digitalWrite(LED_A, LOW);
  delay(80);
  digitalWrite(LED_B, HIGH);
  delay(150);
  digitalWrite(LED_B, LOW);
}
```

### Note Reference Table

```
Octave 3:  C3  CS3 D3  DS3 E3  F3  FS3 G3  GS3 A3  AS3 B3
           131 139 147 156 165 175 185 196 208 220 233 247  Hz

Octave 4:  C4  CS4 D4  DS4 E4  F4  FS4 G4  GS4 A4  AS4 B4
           262 277 294 311 330 349 370 392 415 440 466 494  Hz   ← middle

Octave 5:  C5  CS5 D5  DS5 E5  F5  FS5 G5  GS5 A5  AS5 B5
           523 554 587 622 659 698 740 784 831 880 932 988  Hz
```

Use as: `NOTE_C4`, `NOTE_CS4`, `NOTE_D4`, ... `NOTE_B4`.
`CS` = C sharp, `DS` = D sharp, etc. (the black keys on a piano).

### Ideas to try

| What to do | Code snippet |
|-----------|-------------|
| Single note | `chime.add(&motorA, NOTE_C4, 80, 500); chime.play();` |
| Ascending 3 notes | `chime.add(&motorA, NOTE_C4, 80, 200); chime.add(...` |
| Both motors (chord) | `chime.startStep(); chime.addToStep(&motorA, NOTE_C4, 80); ...` |
| Melody + harmony | Put melody on motorA, harmony on motorB |
| Your own tune! | Look up note frequencies and build a sequence |

---

# Task 2 — `onMotorCommand()` (motor control)

Whenever you drag a throttle slider on the web page, this function is called:

```cpp
void onMotorCommand(char motor, int speed) {
```

| Parameter | Meaning | Example |
|-----------|---------|---------|
| `motor` | Which motor — `'a'` or `'b'` | `'a'` means motor A |
| `speed` | Throttle position | `127` = half forward, `-200` = almost full reverse |

**Speed values:**

```
 -255  ←  -200  ←  -100  ←  0  →  100  →  200  →  +255
full reverse        slow reverse  stop  slow forward     full forward
```

### What you need to do

Translate that speed number into motor commands:

| Speed | What to call | Why |
|-------|-------------|-----|
| **positive** (+1 to +255) | `m->forward(speed)` | Spin forward at that speed |
| **negative** (-1 to -255) | `m->backward(-speed)` | Spin backward *(note: `-speed` converts negative to positive)* |
| **zero** | `m->stop()` | Stop the motor |

**We already wrote the tricky pointer stuff for you:**
```cpp
Motor *m   = (motor == 'b') ? &motorB : &motorA;   // ← given!
int ledPin = (motor == 'b') ? LED_B  : LED_A;       // ← given!
```

- `m` is a pointer to the right `Motor` object. Use `m->forward(...)` etc.
- `ledPin` is the GPIO pin for that motor's LED. Use `digitalWrite(ledPin, HIGH)`.

### Step-by-step

**Step 1 — Forward only**

Start simple: make the motor go forward when speed is positive, stop otherwise.

```cpp
if (speed > 0) {
  m->forward(speed);
} else {
  m->stop();
}
```

**Step 2 — Add backward**

```cpp
if (speed > 0) {
  m->forward(speed);
} else if (speed < 0) {
  m->backward(-speed);     // ← notice the minus sign!
} else {
  m->stop();
}
```

**Step 3 — Add LEDs**

```cpp
if (speed > 0) {
  m->forward(speed);
  digitalWrite(ledPin, HIGH);
} else if (speed < 0) {
  m->backward(-speed);
  digitalWrite(ledPin, HIGH);
} else {
  m->stop();
  digitalWrite(ledPin, LOW);
}
```

**Step 4 — Add logging (helps you debug!)**

```cpp
if (speed > 0) {
  m->forward(speed);
  digitalWrite(ledPin, HIGH);
  Serial.printf("Motor %c → FWD  speed=%d\n", motor, speed);
} else if (speed < 0) {
  m->backward(-speed);
  digitalWrite(ledPin, HIGH);
  Serial.printf("Motor %c → BWD  speed=%d\n", motor, -speed);
} else {
  m->stop();
  digitalWrite(ledPin, LOW);
  Serial.printf("Motor %c → STOP\n", motor);
}
```

Open the Serial Monitor (Tools → Serial Monitor, 115200 baud) and watch the
messages appear as you drag the sliders!

---

### Motor class reference

These are the methods available on `motorA` and `motorB`:

| Method | What it does |
|--------|-------------|
| `forward(0–255)` | Motor spins forward at given speed |
| `backward(0–255)` | Motor spins backward at given speed |
| `stop()` | Motor stops immediately |
| `getSpeed()` | Returns the last speed you set |

Examples:
```cpp
motorA.forward(200);      // motor A forward at 200
motorB.backward(128);     // motor B backward at half speed
motorA.stop();            // stop motor A

int s = motorA.getSpeed();   // read current speed
```

---

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| `forward(-100)` | `forward()` expects 0–255. Use `backward(100)` for reverse. |
| `backward(-50)` | `backward()` expects 0–255. Use `backward(50)`. |
| Forgetting `chime.play()` | Notes queue but nothing plays until you call `chime.play()`! |
| Using `delay()` in chime | `delay()` freezes everything (WiFi disconnects!). Use `ChimePlayer` instead. |
| Typos like `NOTE_C4` vs `NOTE_C_4` | Check the exact constant names in `chime.h`. |

---

## Testing Flow

1. **Task 2 first** — Get the motors spinning with the sliders. It's just 3
   conditions and 3 method calls.
2. **Then Task 1** — Once you can control the motors, make them play a tune
   when START is pressed.
3. **Build + upload often** — Small changes, frequent testing. If something
   breaks, you'll know exactly what caused it.

---

## Bonus: MIDI Easter Egg

You can play any MIDI file through the boat's motors! See the commented block
at the end of `setup()` in `main.cpp` for instructions.

---

Happy boating! ⛵

#include "boat_controller.h"
#include "motor.h"
#include "chime.h"

// ---------------------------------------------------------------------------
// Motor instances
// ---------------------------------------------------------------------------
Motor motorA(MOTOR_A1, MOTOR_A2);
Motor motorB(MOTOR_B1, MOTOR_B2);

// Non-blocking chime player — auto-advances through a note sequence
ChimePlayer chime;

// ---------------------------------------------------------------------------
// ✏️  STUDENT TASK 1: Play a chime sequence when motors are activated.
//
//     onStartMotors() is called once when the user taps the START button
//     on the web page.  Use chime.add() + chime.play() for non-blocking
//     playback so both motors can play overlapping notes.
//
//     Available methods on Motor:
//       forward(int speed)   — pin1 PWM, pin2 LOW  (speed: 0-255)
//       backward(int speed)  — pin1 LOW, pin2 PWM  (speed: 0-255)
//       stop()               — both pins LOW
//       playNote(freq,duty,ms) — start a non-blocking note
//       setFrequency(int)    — change PWM frequency for musical notes
//       note(freq,duty,ms)   — blocking version (freezes loop)
//
//     ChimePlayer methods:
//       chime.add(&motor, freq, duty, ms)  — queue a single note
//       chime.startStep()                   — begin a simultaneous step
//       chime.addToStep(&motor, freq, duty) — add note to current step
//       chime.endStep(durationMs)           — finish this step
//       chime.play()                        — start the whole sequence
//       chime.update()                      — call in loop()
//
//     Simultaneous example (both motors at once for 400ms):
//       chime.startStep();
//       chime.addToStep(&motorA, NOTE_C4, 100);
//       chime.addToStep(&motorB, NOTE_E4, 100);
//       chime.endStep(400);
//       chime.play();
//
//     Pre-defined note constants: NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
//     NOTE_G4, NOTE_A4, NOTE_B4, NOTE_CS4 (C#), NOTE_FS4 (F#), etc.
//     See chime.h for the full list (octaves 3-6).
//
//     LED pins: LED_A (20), LED_B (21)
// ===========================================================================
void onStartMotors() {
  // === ✏️  YOUR CHIME SEQUENCE HERE ===

  // Fanfare: motorA = melody (higher), motorB = harmony (lower third)
  //        Melody     Harmony (third below)
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C4, 100);    // C4
  chime.addToStep(&motorB, NOTE_G3, 80);     // G3  —  perfect 4th below
  chime.endStep(200);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_E4, 100);    // E4
  chime.addToStep(&motorB, NOTE_C4, 80);     // C4  —  minor 3rd below
  chime.endStep(200);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 100);    // G4
  chime.addToStep(&motorB, NOTE_E4, 80);     // E4  —  minor 3rd below
  chime.endStep(300);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C5, 100);    // C5
  chime.addToStep(&motorB, NOTE_G4, 80);     // G4  —  perfect 4th below
  chime.endStep(500);
  chime.play();

  // Blink LEDs in sync
  digitalWrite(LED_A, HIGH);  delay(150);
  digitalWrite(LED_A, LOW);   delay(80);
  digitalWrite(LED_B, HIGH);  delay(150);
  digitalWrite(LED_B, LOW);

  Serial.println("[Motor] Chime started — motors enabled");
}

// ---------------------------------------------------------------------------
// ✏️  STUDENT TASK 2: Map a signed speed (-255..+255) to motor PWM control.
//
//     motor  : 'a' or 'b'
//     speed  : -255 = full backward  →  0 = stop  →  +255 = full forward
//
//     Available methods on Motor:
//       forward(int speed)   — pin1 PWM, pin2 LOW  (speed: 0-255)
//       backward(int speed)  — pin1 LOW, pin2 PWM  (speed: 0-255)
//       stop()               — both pins LOW
//       note(freq,duty,ms)   — play a frequency note for ms, then stop
//       playNote(f,d,m)      — non-blocking version
//       setFrequency(int)    — change PWM frequency for musical notes
//       getSpeed()           — returns last set speed
//
//     Pre-defined note constants: NOTE_C4, NOTE_E4, NOTE_G4, etc.
//
//     LED pins: LED_A (20), LED_B (21)
//     Use digitalWrite(pin, HIGH/LOW) to light the corresponding LED.
// ===========================================================================
void onMotorCommand(char motor, int speed) {
  // === ✏️  YOUR MOTOR MAPPING CODE HERE ===

  Motor *m = (motor == 'b') ? &motorB : &motorA;
  int ledPin = (motor == 'b') ? LED_B : LED_A;

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
}

// ---------------------------------------------------------------------------
// Setup & Loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_A, HIGH);   // LED on during startup
  digitalWrite(LED_B, LOW);

  // Brief USB CDC wait (250 ms is usually enough on CDC-with-wait-loop)
  for (int i = 0; i < 4; i++) {
    delay(50);
    if (Serial) break;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("       ⛵  INNOX BOAT CONTROLLER");
  Serial.println("========================================");
  Serial.flush();

  setupBoatController();

  // Startup chime: motorA = melody, motorB = harmony (mostly a third below)
  //        Melody       Harmony
  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 80);     // G4
  chime.addToStep(&motorB, NOTE_E4, 65);     // E4  —  minor 3rd below
  chime.endStep(400);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_F4, 80);     // F4
  chime.addToStep(&motorB, NOTE_D4, 65);     // D4  —  minor 3rd below
  chime.endStep(400);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_E4, 80);     // E4
  chime.addToStep(&motorB, NOTE_C4, 65);     // C4  —  major 3rd below
  chime.endStep(400);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 80);     // G4
  chime.addToStep(&motorB, NOTE_E4, 65);     // E4  —  minor 3rd below
  chime.endStep(600);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C5, 80);     // C5
  chime.addToStep(&motorB, NOTE_G4, 65);     // G4  —  perfect 4th below
  chime.endStep(600);
  chime.play();

  // === MIDI song easter egg ===
  // Uncomment the block below to play a converted MIDI song instead of the
  // startup chime.  Regenerate src/song.h from a .mid file using:
  //   python tools/midi_to_chime.py tools/your_song.mid
  //
  // #include "song.h"
  // Motor* trackMap[] = { &motorA, &motorB };
  // chime.loadSong(&songData, trackMap);
  // chime.play();

  // The banner will keep repeating every 3 s in loopBoatController() until
  // a client connects, so you never miss the SSID/IP even with USB CDC lag.

  digitalWrite(LED_A, LOW);
}

void loop() {
  // Advance any playing chime notes
  motorA.update();
  motorB.update();
  chime.update();

  loopBoatController();
}
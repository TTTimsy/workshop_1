#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Motor PWM Pins — Seeed XIAO ESP32C3
// ---------------------------------------------------------------------------
#define MOTOR_A1 5
#define MOTOR_A2 6
#define MOTOR_B1 8
#define MOTOR_B2 7

#define LED_A 20
#define LED_B 21

// ---------------------------------------------------------------------------
// Motor class
//  Controls one DC motor via two PWM pins (H-bridge).
//  forward(speed)  → pin1 PWM, pin2 LOW
//  backward(speed) → pin1 LOW, pin2 PWM
//  stop()          → both pins LOW
// ---------------------------------------------------------------------------
class Motor {
private:
  int pin1;
  int pin2;
  int channel1;
  int channel2;
  int currentSpeed;
  int currentFreq;

  unsigned long _noteEndTime;   // 0 = no note playing
  int           _noteDuty;

  static constexpr int PWM_RES  = 8;
  static constexpr int DEF_FREQ = 5000;

public:
  Motor(int p1, int p2)
    : pin1(p1), pin2(p2), currentSpeed(0), currentFreq(DEF_FREQ),
      _noteEndTime(0), _noteDuty(0) {
    static int nextChannel = 0;
    channel1 = nextChannel++;
    channel2 = nextChannel++;
    ledcSetup(channel1, DEF_FREQ, PWM_RES);
    ledcSetup(channel2, DEF_FREQ, PWM_RES);
    ledcAttachPin(pin1, channel1);
    ledcAttachPin(pin2, channel2);
  }

  // Change PWM frequency for both channels (e.g. for musical chime notes)
  void setFrequency(int freqHz) {
    currentFreq = freqHz;
    ledcChangeFrequency(channel1, freqHz, PWM_RES);
    ledcChangeFrequency(channel2, freqHz, PWM_RES);
  }

  // --- Non-blocking note -----------------------------------------------
  //  Start playing a frequency note now.  It will auto-stop after ms.
  //  Call update() every loop() to check for expiry.
  void playNote(int freqHz, int duty, int durationMs) {
    setFrequency(freqHz);
    _noteDuty = constrain(duty, 0, 255);
    _noteEndTime = millis() + durationMs;
    ledcWrite(channel1, _noteDuty);
    ledcWrite(channel2, 0);
    currentSpeed = _noteDuty;
  }

  // Call every loop() — stops the note when its duration expires
  void update() {
    if (_noteEndTime != 0 && millis() >= _noteEndTime) {
      _noteEndTime = 0;
      _noteDuty = 0;
      currentSpeed = 0;
      ledcWrite(channel1, 0);
      ledcWrite(channel2, 0);
    }
  }

  // Check if a note is currently playing
  bool isPlaying() const { return _noteEndTime != 0; }

  // Cancel any playing note immediately
  void stopNote() {
    _noteEndTime = 0;
    _noteDuty = 0;
    currentSpeed = 0;
    ledcWrite(channel1, 0);
    ledcWrite(channel2, 0);
  }

  // --- Blocking helpers (kept for convenience) --------------------------
  //  Play a note and block with delay() — simpler but freezes loop.
  void note(int freqHz, int duty, int durationMs) {
    playNote(freqHz, duty, durationMs);
    delay(durationMs);
    update();
  }

  void forward(int speed = 255) {
    if (_noteEndTime) stopNote();                           // cancel any playing note
    if (currentFreq < DEF_FREQ) setFrequency(DEF_FREQ);    // reset from chime mode
    currentSpeed = constrain(speed, 0, 255);
    ledcWrite(channel1, currentSpeed);
    ledcWrite(channel2, 0);
  }

  void backward(int speed = 255) {
    if (_noteEndTime) stopNote();                           // cancel any playing note
    if (currentFreq < DEF_FREQ) setFrequency(DEF_FREQ);    // reset from chime mode
    currentSpeed = constrain(speed, 0, 255);
    ledcWrite(channel1, 0);
    ledcWrite(channel2, currentSpeed);
  }

  void stop() {
    currentSpeed = 0;
    ledcWrite(channel1, 0);
    ledcWrite(channel2, 0);
  }

  int  getSpeed() const { return currentSpeed; }
  int  getFrequency() const { return currentFreq; }
};

#endif // MOTOR_H

#ifndef DRIVE_SYSTEM_H
#define DRIVE_SYSTEM_H

#include <Arduino.h>
#include "motor.h"

// DriveSystem owns driving policy:
// web sliders give desired A/B thrust, then DriveSystem applies deadzone,
// racing duty mapping, non-blocking start kick, slew limiting, turn limiting,
// and reversal protection before touching the motors.
class DriveSystem {
public:
  DriveSystem(Motor &motorA, Motor &motorB, int ledA, int ledB);

  void setTarget(int speedA, int speedB);
  void setMotorTarget(char motor, int speed);
  void update();
  void stop();

  int targetA() const { return _sideA.targetInput; }
  int targetB() const { return _sideB.targetInput; }
  int outputA() const { return _sideA.outputDuty; }
  int outputB() const { return _sideB.outputDuty; }

private:
  struct SideState {
    int targetInput = 0;          // cleaned slider target, -255..255
    int targetDuty = 0;           // mapped PWM duty with sign, -255..255
    int currentDuty = 0;          // slew-limited duty without temporary kick
    int outputDuty = 0;           // actual duty sent to Motor, includes kick
    unsigned long kickUntil = 0;
    int kickDirection = 0;
    unsigned long reverseHoldUntil = 0;
  };

  Motor *_motorA;
  Motor *_motorB;
  int _ledA;
  int _ledB;
  int _rawTargetA;
  int _rawTargetB;
  unsigned long _lastUpdateAt;

  void refreshTargets();
  void applyTurnLimit(int inputA, int inputB, int &limitedA, int &limitedB) const;
  int cleanInput(int value) const;
  int mapInputToDuty(int value) const;
  void updateSide(SideState &side, Motor &motor, int ledPin, unsigned long now);
  void writeSide(SideState &side, Motor &motor, int ledPin, int signedDuty);
  static int signOf(int value);
  static int approach(int current, int target, int step);

  SideState _sideA;
  SideState _sideB;
};

#endif // DRIVE_SYSTEM_H

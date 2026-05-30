#include "drive_system.h"

DriveSystem::DriveSystem(Motor &motorA, Motor &motorB, int ledA, int ledB)
  : _motorA(&motorA),
    _motorB(&motorB),
    _ledA(ledA),
    _ledB(ledB),
    _rawTargetA(0),
    _rawTargetB(0),
    _lastUpdateAt(0) {}

void DriveSystem::setTarget(int speedA, int speedB) {
  _rawTargetA = constrain(speedA, -255, 255);
  _rawTargetB = constrain(speedB, -255, 255);
  refreshTargets();
}

void DriveSystem::setMotorTarget(char motor, int speed) {
  if (motor == 'b' || motor == 'B') {
    _rawTargetB = constrain(speed, -255, 255);
  } else {
    _rawTargetA = constrain(speed, -255, 255);
  }
  refreshTargets();
}

void DriveSystem::refreshTargets() {
  int limitedA = 0;
  int limitedB = 0;
  applyTurnLimit(cleanInput(_rawTargetA), cleanInput(_rawTargetB), limitedA, limitedB);
  _sideA.targetInput = limitedA;
  _sideB.targetInput = limitedB;
}

void DriveSystem::update() {
  unsigned long now = millis();
  if (_lastUpdateAt != 0 && now - _lastUpdateAt < DRIVE_UPDATE_INTERVAL_MS) {
    return;
  }
  _lastUpdateAt = now;

  updateSide(_sideA, *_motorA, _ledA, MOTOR_A_DRIVE_POLARITY, now);
  updateSide(_sideB, *_motorB, _ledB, MOTOR_B_DRIVE_POLARITY, now);
}

void DriveSystem::stop() {
  _rawTargetA = 0;
  _rawTargetB = 0;
  _sideA = SideState();
  _sideB = SideState();
  _motorA->stop();
  _motorB->stop();
  digitalWrite(_ledA, LOW);
  digitalWrite(_ledB, LOW);
}

void DriveSystem::applyTurnLimit(int inputA, int inputB,
                                 int &limitedA, int &limitedB) const {
  int common = (inputA + inputB) / 2;
  int diff = (inputA - inputB) / 2;
  int diffLimit = DRIVE_SPIN_INPUT_LIMIT;
  int absCommon = abs(common);

  if (absCommon >= DRIVE_HIGH_SPEED_2) {
    diffLimit = DRIVE_DIFF_LIMIT_2;
  } else if (absCommon >= DRIVE_HIGH_SPEED_1) {
    diffLimit = DRIVE_DIFF_LIMIT_1;
  }

  diff = constrain(diff, -diffLimit, diffLimit);
  limitedA = constrain(common + diff, -255, 255);
  limitedB = constrain(common - diff, -255, 255);
}

int DriveSystem::cleanInput(int value) const {
  value = constrain(value, -255, 255);
  if (abs(value) < DRIVE_DEADZONE) return 0;
  return value;
}

int DriveSystem::mapInputToDuty(int value) const {
  int input = cleanInput(value);
  int magnitude = abs(input);
  if (magnitude == 0) return 0;

  float t = (float)(magnitude - DRIVE_DEADZONE) / (float)(255 - DRIVE_DEADZONE);
  t = constrain(t, 0.0f, 1.0f);
  float curved = t * t * (3.0f - 2.0f * t);  // smoothstep: stable near min/max
  int duty = DRIVE_MIN_DUTY +
             (int)((DRIVE_MAX_DUTY - DRIVE_MIN_DUTY) * curved + 0.5f);
  duty = constrain(duty, DRIVE_MIN_DUTY, DRIVE_MAX_DUTY);
  return (input > 0) ? duty : -duty;
}

void DriveSystem::updateSide(SideState &side, Motor &motor,
                             int ledPin, int polarity, unsigned long now) {
  side.targetDuty = mapInputToDuty(side.targetInput);

  int target = side.targetDuty;
  int targetSign = signOf(target);
  int currentSign = signOf(side.currentDuty);

  if (targetSign == 0) {
    side.kickUntil = 0;
    side.kickDirection = 0;
    side.reverseHoldUntil = 0;

    if (abs(side.currentDuty) <= DRIVE_MIN_DUTY) {
      side.currentDuty = 0;
    } else {
      side.currentDuty = approach(side.currentDuty, 0, DRIVE_DECEL_STEP);
    }
  } else if (currentSign != 0 && currentSign != targetSign) {
    side.kickUntil = 0;
    side.kickDirection = 0;

    if (abs(side.currentDuty) <= DRIVE_MIN_DUTY) {
      side.currentDuty = 0;
    } else {
      side.currentDuty = approach(side.currentDuty, 0, DRIVE_DECEL_STEP);
    }

    if (side.currentDuty == 0 && side.reverseHoldUntil == 0) {
      side.reverseHoldUntil = now + DRIVE_REVERSE_BRAKE_MS;
    }
  } else if (side.reverseHoldUntil != 0 && now < side.reverseHoldUntil) {
    side.currentDuty = 0;
  } else {
    side.reverseHoldUntil = 0;

    if (side.currentDuty == 0) {
      side.currentDuty = targetSign * min(abs(target), DRIVE_MIN_DUTY);
      side.kickUntil = now + DRIVE_START_KICK_MS;
      side.kickDirection = targetSign;
    } else {
      int step = (abs(target) > abs(side.currentDuty)) ? DRIVE_ACCEL_STEP
                                                       : DRIVE_DECEL_STEP;
      side.currentDuty = approach(side.currentDuty, target, step);
    }
  }

  int command = side.currentDuty;
  if (side.kickUntil != 0) {
    if (now < side.kickUntil && signOf(command) == side.kickDirection) {
      command = side.kickDirection * DRIVE_START_KICK_DUTY;
    } else {
      side.kickUntil = 0;
      side.kickDirection = 0;
    }
  }

  writeSide(side, motor, ledPin, command, polarity);
}

void DriveSystem::writeSide(SideState &side, Motor &motor,
                            int ledPin, int signedDuty, int polarity) {
  signedDuty = constrain(signedDuty, -DRIVE_MAX_DUTY, DRIVE_MAX_DUTY);
  int electricalDuty = signedDuty * ((polarity < 0) ? -1 : 1);
  side.outputDuty = electricalDuty;
  motor.driveDuty(electricalDuty);
  digitalWrite(ledPin, electricalDuty == 0 ? LOW : HIGH);
}

int DriveSystem::signOf(int value) {
  if (value > 0) return 1;
  if (value < 0) return -1;
  return 0;
}

int DriveSystem::approach(int current, int target, int step) {
  if (current < target) return min(current + step, target);
  if (current > target) return max(current - step, target);
  return current;
}

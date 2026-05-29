#ifndef UPLOAD_SAFETY_H
#define UPLOAD_SAFETY_H

#include <Arduino.h>

class ChimePlayer;
class Motor;

void setupUploadSafety(Motor *motorA,
                       Motor *motorB,
                       ChimePlayer *chime,
                       int ledA,
                       int ledB);
void loopUploadSafety();
bool isUploadSafetyMode();
bool blockStartForUploadSafety();
bool blockMotorCommandForUploadSafety(int speed);

#endif // UPLOAD_SAFETY_H

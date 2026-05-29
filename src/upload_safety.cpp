#include "upload_safety.h"

#include "chime.h"
#include "motor.h"

namespace {
Motor *s_motorA = nullptr;
Motor *s_motorB = nullptr;
ChimePlayer *s_chime = nullptr;
int s_ledA = -1;
int s_ledB = -1;

bool s_uploadSafetyMode = false;
bool s_uploadIgnoreLogged = false;
char s_commandBuffer[48];
size_t s_commandLength = 0;

void prepareForUpload() {
  if (!s_motorA || !s_motorB || !s_chime) {
    Serial.println("[UPLOAD] Safety module not configured");
    return;
  }

  s_uploadSafetyMode = true;
  s_uploadIgnoreLogged = false;

  Serial.println("[UPLOAD] Preparing motor pins for bootloader...");
  if (s_chime->isPlaying()) s_chime->clear();

  s_motorA->stop();
  s_motorB->stop();
  if (s_ledA >= 0) digitalWrite(s_ledA, LOW);
  if (s_ledB >= 0) digitalWrite(s_ledB, LOW);
  delay(30);

  s_motorA->safeReleasePins();
  s_motorB->safeReleasePins();
  delay(30);

  Serial.println("[UPLOAD] READY_FOR_UPLOAD");
  Serial.flush();
}

void handleCommandLine(const char *line) {
  String cmd(line);
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.length() == 0) return;

  if (cmd == "prepare_upload" || cmd == "safe_upload" || cmd == "upload") {
    prepareForUpload();
    return;
  }

  Serial.printf("[SERIAL] Unknown command: %s\n", cmd.c_str());
}
} // namespace

void setupUploadSafety(Motor *motorA,
                       Motor *motorB,
                       ChimePlayer *chime,
                       int ledA,
                       int ledB) {
  s_motorA = motorA;
  s_motorB = motorB;
  s_chime = chime;
  s_ledA = ledA;
  s_ledB = ledB;
}

void loopUploadSafety() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      s_commandBuffer[s_commandLength] = '\0';
      handleCommandLine(s_commandBuffer);
      s_commandLength = 0;
      continue;
    }

    if (s_commandLength < sizeof(s_commandBuffer) - 1) {
      s_commandBuffer[s_commandLength++] = c;
    } else {
      s_commandLength = 0;
      Serial.println("[SERIAL] Command too long; buffer cleared");
    }
  }
}

bool isUploadSafetyMode() {
  return s_uploadSafetyMode;
}

bool blockStartForUploadSafety() {
  if (!s_uploadSafetyMode) return false;

  Serial.println("[UPLOAD] START ignored while motor pins are released");
  return true;
}

bool blockMotorCommandForUploadSafety(int speed) {
  if (!s_uploadSafetyMode) return false;

  if (speed != 0 && !s_uploadIgnoreLogged) {
    Serial.println("[UPLOAD] Motor commands ignored while motor pins are released");
    s_uploadIgnoreLogged = true;
  }
  return true;
}

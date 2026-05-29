#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include "boat_config.h"

// motor.h 封装单个直流电机：
// 两个 PWM 引脚接到 H 桥，pin1 输出 PWM 表示正转，pin2 输出 PWM 表示反转。
// 同一个 PWM 系统也被复用来改变频率，让电机线圈发出简单音调。

// ---------------------------------------------------------------------------
// Motor PWM Pins — Seeed XIAO ESP32C3
// ---------------------------------------------------------------------------
// A 电机的两个 H 桥输入脚。
#define MOTOR_A1 5
#define MOTOR_A2 6
// B 电机的两个 H 桥输入脚。
#define MOTOR_B1 8
#define MOTOR_B2 7

// 两个板载/外接 LED，用来提示 A/B 电机是否正在工作。
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
  // 控制 H 桥的两个 GPIO。
  int pin1;
  int pin2;
  // ESP32 LEDC PWM 通道编号，每个 GPIO 对应一个通道。
  int channel1;
  int channel2;
  // 记录最近一次设置的速度或音符 duty。
  int currentSpeed;
  // 记录当前方向：1 前进，-1 后退，0 停止/播放音符。
  int currentDirection;
  // 记录当前 PWM 频率；普通电机控制使用 DEF_FREQ，音符播放使用音高频率。
  int currentFreq;

  // 音符结束时间；0 表示当前没有音符在播放。
  unsigned long _noteEndTime;   // 0 = no note playing
  // 当前音符的 PWM 占空比，影响电机发声音量/力度。
  int           _noteDuty;
  // true 表示 GPIO 当前挂在 LEDC PWM 通道上；烧录前安全释放后会变 false。
  bool          _pinsAttached;

  // PWM_RES=8 表示占空比范围 0-255；DEF_FREQ=1kHz 是常规电机 PWM 频率。
  // 小直流电机在 20kHz 下可能启动扭矩不够；1kHz 更容易让电机实际转起来。
  static constexpr int PWM_RES  = 8;
  static constexpr int DEF_FREQ = 1000;

  void attachPwmPins() {
    if (_pinsAttached) return;

    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
    ledcAttachPin(pin1, channel1);
    ledcAttachPin(pin2, channel2);
    _pinsAttached = true;
    ledcWrite(channel1, 0);
    ledcWrite(channel2, 0);
  }

  int mapDriveDuty(int speed) const {
    int targetSpeed = constrain(speed, 0, 255);
    if (targetSpeed < DRIVE_DEADZONE) return 0;
    return (int)map(targetSpeed, DRIVE_DEADZONE, 255,
                    DRIVE_MIN_DUTY, DRIVE_MAX_DUTY);
  }

public:
  // 构造函数保存引脚、分配 PWM 通道、连接引脚并立即停止电机。
  Motor(int p1, int p2)
    : pin1(p1), pin2(p2), currentSpeed(0), currentDirection(0), currentFreq(DEF_FREQ),
      _noteEndTime(0), _noteDuty(0), _pinsAttached(false) {
    static int nextChannel = 0;
    channel1 = nextChannel++;
    channel2 = nextChannel++;
    ledcSetup(channel1, DEF_FREQ, PWM_RES);
    ledcSetup(channel2, DEF_FREQ, PWM_RES);
    attachPwmPins();
    stop();    // ensure pins start LOW, no stray signals
  }

  // Change PWM frequency for both channels (e.g. for musical chime notes)
  void setFrequency(int freqHz) {
    attachPwmPins();
    if (currentFreq == freqHz) return;
    // 两个方向通道保持相同频率，避免正反转切换时频率不一致。
    currentFreq = freqHz;
    ledcChangeFrequency(channel1, freqHz, PWM_RES);
    ledcChangeFrequency(channel2, freqHz, PWM_RES);
  }

  // --- Non-blocking note -----------------------------------------------
  //  Start playing a frequency note now.  It will auto-stop after ms.
  //  Call update() every loop() to check for expiry.
  void playNote(int freqHz, int duty, int durationMs) {
    // 把 PWM 频率切到音符频率。
    setFrequency(freqHz);
    // 限制 duty 在 0-255，保护 PWM 输入范围。
    _noteDuty = constrain(duty, 0, 255);
    // 记录将来应该自动停止的时间点。
    _noteEndTime = millis() + durationMs;
    // 音符只从 channel1 输出，channel2 保持低电平，避免 H 桥短路风险。
    ledcWrite(channel1, _noteDuty);
    ledcWrite(channel2, 0);
    currentSpeed = _noteDuty;
  }

  // Call every loop() — stops the note when its duration expires
  void update() {
    if (_noteEndTime != 0 && millis() >= _noteEndTime) {
      // 音符到期后清状态并把两个 PWM 输出关掉。
      _noteEndTime = 0;
      _noteDuty = 0;
      currentSpeed = 0;
      currentDirection = 0;
      if (_pinsAttached) {
        ledcWrite(channel1, 0);
        ledcWrite(channel2, 0);
      }
    }
  }

  // Check if a note is currently playing
  bool isPlaying() const { return _noteEndTime != 0; }

  // Cancel any playing note immediately
  void stopNote() {
    // 立即取消音符，供 forward()/backward() 抢占使用。
    _noteEndTime = 0;
    _noteDuty = 0;
    currentSpeed = 0;
    currentDirection = 0;
    if (_pinsAttached) {
      ledcWrite(channel1, 0);
      ledcWrite(channel2, 0);
    }
  }

  // --- Blocking helpers (kept for convenience) --------------------------
  //  Play a note and block with delay() — simpler but freezes loop.
  void note(int freqHz, int duty, int durationMs) {
    // 简单阻塞版：播放后 delay 等待，不适合需要保持 Wi-Fi 响应的主循环。
    playNote(freqHz, duty, durationMs);
    delay(durationMs);
    update();
  }

  void forwardDuty(int duty = 255) {
    // DriveSystem 调用这个函数时，duty 已经是最终 PWM，占空比不再二次映射。
    if (_noteEndTime) stopNote();                           // cancel any playing note
    setFrequency(DEF_FREQ);
    currentSpeed = constrain(duty, 0, 255);
    currentDirection = (currentSpeed > 0) ? 1 : 0;
    ledcWrite(channel1, currentSpeed);
    ledcWrite(channel2, 0);
  }

  void backwardDuty(int duty = 255) {
    // DriveSystem 调用这个函数时，duty 已经是最终 PWM，占空比不再二次映射。
    if (_noteEndTime) stopNote();                           // cancel any playing note
    setFrequency(DEF_FREQ);
    currentSpeed = constrain(duty, 0, 255);
    currentDirection = (currentSpeed > 0) ? -1 : 0;
    ledcWrite(channel1, 0);
    ledcWrite(channel2, currentSpeed);
  }

  void driveDuty(int signedDuty) {
    signedDuty = constrain(signedDuty, -255, 255);
    if (signedDuty > 0) {
      forwardDuty(signedDuty);
    } else if (signedDuty < 0) {
      backwardDuty(-signedDuty);
    } else {
      stop();
    }
  }

  void forward(int speed = 255) {
    // Legacy helper: speed 是用户输入强度，先映射到有效 PWM，再输出。
    forwardDuty(mapDriveDuty(speed));
  }

  void backward(int speed = 255) {
    // Legacy helper: speed 是用户输入强度，先映射到有效 PWM，再输出。
    backwardDuty(mapDriveDuty(speed));
  }

  void stop() {
    // 停止就是两个 H 桥输入都拉低，让电机不再主动驱动。
    currentSpeed = 0;
    currentDirection = 0;
    if (_pinsAttached) {
      ledcWrite(channel1, 0);
      ledcWrite(channel2, 0);
    }
  }

  void safeReleasePins() {
    // 烧录前使用：停止 PWM，并尽量把引脚主动保持在低电平。
    // 注意：进入 bootloader 以后 MCU 会 reset，固件无法继续保证 GPIO 状态。
    _noteEndTime = 0;
    _noteDuty = 0;
    currentSpeed = 0;
    currentDirection = 0;

    attachPwmPins();
    ledcWrite(channel1, 0);
    ledcWrite(channel2, 0);
    delayMicroseconds(200);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
  }

  int  getSpeed() const { return currentSpeed; }
  int  getFrequency() const { return currentFreq; }
};

#endif // MOTOR_H

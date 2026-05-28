#include "boat_controller.h"
#include "motor.h"
#include "chime.h"

// main.cpp 是学生主要动手的入口文件：
// 1. 定义两路电机对象 motorA/motorB。
// 2. 定义旋律播放器 chime。
// 3. 在 onStartMotors() 和 onMotorCommand() 里填写启动音效和油门控制逻辑。

// ---------------------------------------------------------------------------
// Motor instances
// ---------------------------------------------------------------------------
// motorA 使用 MOTOR_A1/MOTOR_A2 两个 PWM 引脚控制 A 电机的正反转。
Motor motorA(MOTOR_A1, MOTOR_A2);
// motorB 使用 MOTOR_B1/MOTOR_B2 两个 PWM 引脚控制 B 电机的正反转。
Motor motorB(MOTOR_B1, MOTOR_B2);

// Non-blocking chime player — auto-advances through a note sequence
// chime 是非阻塞旋律队列；loop() 每次调用 chime.update() 推进下一拍。
ChimePlayer chime;

// ===========================================================================
//  ✏️  TASK 1: Play a chime when START is pressed
// ===========================================================================
void onStartMotors() {
  // ✏️  Write your chime here. See README.md for step-by-step examples.
  //     Queue notes with chime.add() / chime.startStep(), then chime.play().

  // 这里目前只打印日志；练习目标是让学生在这里加入 chime.add()/chime.play()。
  Serial.println("[Motor] Chime started — motors enabled");
}

// ===========================================================================
//  ✏️  TASK 2: Control motor speed & direction from throttle sliders
// ===========================================================================
void onMotorCommand(char motor, int speed) {
  // These are given to you — no pointers needed!
  // 根据网页传来的 motor 字符选择 A 或 B 电机；不是 'b' 时默认 A。
  Motor *m   = (motor == 'b') ? &motorB : &motorA;
  // 同样根据电机选择对应 LED，方便用灯显示哪路电机正在动。
  int ledPin = (motor == 'b') ? LED_B  : LED_A;

  // ✏️  Fill in the logic below.  See README.md for step-by-step examples.
  //     speed > 0  →  m->forward(speed)   + LED on
  //     speed < 0  →  m->backward(-speed)  + LED on
  //     speed == 0 →  m->stop()            + LED off

  if (speed > 0) {
    // speed 为正数时应该调用 m->forward(speed)，并点亮对应 LED。

  } else if (speed < 0) {
    // speed 为负数时应该调用 m->backward(-speed)，负负得正得到 PWM 占空比。

  } else {
    // speed 为 0 时应该停止电机并熄灭对应 LED。

  }
}

// ---------------------------------------------------------------------------
// Startup chime — plays a short descending harmony on boot
// ---------------------------------------------------------------------------
void playStartupChime() {
  // 每个 startStep()/endStep() 表示一个同时播放的和弦步骤。
  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 50);     // G4
  chime.addToStep(&motorB, NOTE_E4, 50);     // E4  —  minor 3rd below
  chime.endStep(200);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_F4, 50);     // F4
  chime.addToStep(&motorB, NOTE_D4, 50);     // D4  —  minor 3rd below
  chime.endStep(200);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_E4, 50);     // E4
  chime.addToStep(&motorB, NOTE_C4, 50);     // C4  —  major 3rd below
  chime.endStep(600);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 50);     // G4
  chime.addToStep(&motorB, NOTE_E4, 50);     // E4  —  minor 3rd below
  chime.endStep(600);
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C5, 50);     // C5
  chime.addToStep(&motorB, NOTE_G4, 50);     // G4  —  perfect 4th below
  chime.endStep(600);
  chime.play();
}

// ---------------------------------------------------------------------------
// Setup & Loop
// ---------------------------------------------------------------------------
void setup() {
  // 打开 USB 串口，方便在 Serial Monitor 里看调试输出。
  Serial.begin(115200);

  // LED_A/LED_B 是普通 GPIO 输出，用来提示电机状态。
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_A, LOW);    // both LEDs start OFF
  digitalWrite(LED_B, LOW);

  // Brief USB CDC wait (250 ms is usually enough on CDC-with-wait-loop)
  for (int i = 0; i < 4; i++) {
    // 给 USB CDC 串口一点时间枚举，避免开机 banner 太早打印看不到。
    delay(50);
    if (Serial) break;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("       ⛵  INNOX BOAT CONTROLLER");
  Serial.println("========================================");
  Serial.flush();

  // 启动 Wi-Fi 热点、HTTP 页面服务器和 WebSocket 服务器。
  setupBoatController();

  digitalWrite(LED_A, HIGH);   // LED on during startup chime
  playStartupChime();
  digitalWrite(LED_A, LOW);    // startup chime scheduled, LED off

  // === MIDI song easter egg ===
  // Uncomment the block below to play a converted MIDI song instead of the
  // startup chime.  Regenerate src/song.h from a .mid file using:
  //   python tools/midi_to_chime.py tools/your_song.mid
  //
  // #include "song.h"
  // Motor* trackMap[] = { &motorA, &motorB };
  // chime.loadSong(&songData, trackMap);
  // chime.play();

  digitalWrite(LED_A, LOW);    // startup chime scheduled, LED off

  // The banner will keep repeating every 3 s in loopBoatController() until
  // a client connects, so you never miss the SSID/IP even with USB CDC lag.
}

void loop() {
  // Advance any playing chime notes
  // 电机和旋律都是非阻塞的，必须在 loop() 里不断 update。
  motorA.update();
  motorB.update();
  chime.update();

  // 处理网页请求、WebSocket 消息和串口连接提示。
  loopBoatController();
}

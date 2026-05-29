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

  // === TASK 1 SOLUTION CODE - ACTIVE FOR UPLOAD ===
  chime.clear();                             // 清空开机旋律留下的队列，确保 START 只播放下面这段旋律。
  // 下面这些行已经启用：按网页 START 按钮时，两路电机会播放三段启动和弦。
  chime.startStep();                         // 开始第 1 个和弦步骤，让后面加入的音符同时播放。
  chime.addToStep(&motorA, NOTE_C4, 80);      // 让 motorA 在第 1 步播放 C4，力度/duty 为 80。
  chime.addToStep(&motorB, NOTE_E4, 60);      // 让 motorB 在第 1 步播放 E4，力度稍小形成和声。
  chime.endStep(250);                         // 结束第 1 步，并让这一组音符持续 250 毫秒。
  chime.startStep();                         // 开始第 2 个和弦步骤。
  chime.addToStep(&motorA, NOTE_E4, 80);      // 让 motorA 在第 2 步播放 E4。
  chime.addToStep(&motorB, NOTE_G4, 60);      // 让 motorB 在第 2 步播放 G4，继续形成和声。
  chime.endStep(250);                         // 结束第 2 步，并持续 250 毫秒。
  chime.startStep();                         // 开始第 3 个和弦步骤。
  chime.addToStep(&motorA, NOTE_G4, 90);      // 让 motorA 在第 3 步播放 G4，力度略强作为收尾。
  chime.addToStep(&motorB, NOTE_C5, 70);      // 让 motorB 在第 3 步播放 C5，让结尾更明亮。
  chime.endStep(450);                         // 结束第 3 步，并持续 450 毫秒。
  chime.play();                               // 开始播放上面排好的所有旋律步骤；没有这一行就不会发声。
  // === END TASK 1 SOLUTION CODE ===

  // 打印日志，方便在串口监视器中确认 START 命令已经触发。
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
    // === TASK 2 SOLUTION CODE - ACTIVE FOR UPLOAD: FORWARD ===
    m->forward(speed);                    // 让当前选中的电机按正数 speed 前进。
    digitalWrite(ledPin, HIGH);           // 电机运行时点亮对应 LED，提示这一路电机正在工作。
    Serial.printf("Motor %c -> FWD  speed=%d\n", motor, speed);  // 在串口打印前进方向和速度，方便调试。
    // === END TASK 2 FORWARD CODE ===

  } else if (speed < 0) {
    // speed 为负数时应该调用 m->backward(-speed)，负负得正得到 PWM 占空比。
    // === TASK 2 SOLUTION CODE - ACTIVE FOR UPLOAD: BACKWARD ===
    m->backward(-speed);                  // speed 是负数，取 -speed 变成正数后让电机后退。
    digitalWrite(ledPin, HIGH);           // 电机后退时也点亮对应 LED。
    Serial.printf("Motor %c -> BWD  speed=%d\n", motor, -speed); // 在串口打印后退方向和实际 PWM 速度。
    // === END TASK 2 BACKWARD CODE ===

  } else {
    // speed 为 0 时应该停止电机并熄灭对应 LED。
    // === TASK 2 SOLUTION CODE - ACTIVE FOR UPLOAD: STOP ===
    m->stop();                            // 停止当前选中的电机。
    digitalWrite(ledPin, LOW);            // 电机停止后熄灭对应 LED。
    Serial.printf("Motor %c -> STOP\n", motor); // 在串口打印停止状态，确认松开滑杆后命令已发送。
    // === END TASK 2 STOP CODE ===

  }
}

// ---------------------------------------------------------------------------
// Startup chime — plays a short descending harmony on boot
// ---------------------------------------------------------------------------
void playStartupChime() {
  chime.clear();                             // 开机旋律单独成队列，避免和后续 START 旋律混在一起。
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

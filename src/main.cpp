#include "boat_controller.h"
#include "boat_config.h"
#include "motor.h"
#include "drive_system.h"
#include "chime.h"
#include "upload_safety.h"

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

// DriveSystem 是驾驶中间层：网页双杆只给目标，平滑、起步冲量、转向限制都在这里完成。
DriveSystem driveSystem(motorA, motorB, LED_A, LED_B);

// Non-blocking chime player — auto-advances through a note sequence
// chime 是非阻塞旋律队列；loop() 每次调用 chime.update() 推进下一拍。
ChimePlayer chime;

void stopAllMotors() {
  if (chime.isPlaying()) chime.clear();
  driveSystem.stop();
  Serial.println("[Motor] SAFETY STOP - both motors stopped");
}

// ===========================================================================
//  ✏️  TASK 1: Play a chime when START is pressed
// ===========================================================================
void onStartMotors() {
  if (blockStartForUploadSafety()) return;
  driveSystem.stop();

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
  if (blockMotorCommandForUploadSafety(speed)) return;

  speed = constrain(speed, -255, 255);
  if (abs(speed) < DRIVE_DEADZONE) speed = 0;

  // 滑杆控制优先级最高：一旦用户真正给油，就停止还在播放的启动旋律。
  if (speed == 0 && chime.isPlaying()) return;
  if (speed != 0 && chime.isPlaying()) chime.clear();

  driveSystem.setMotorTarget(motor, speed);
}

void onDriveCommand(int speedA, int speedB) {
  int maxSpeed = max(abs(speedA), abs(speedB));
  if (blockMotorCommandForUploadSafety(maxSpeed)) return;

  speedA = constrain(speedA, -255, 255);
  speedB = constrain(speedB, -255, 255);
  if (abs(speedA) < DRIVE_DEADZONE) speedA = 0;
  if (abs(speedB) < DRIVE_DEADZONE) speedB = 0;

  if (speedA == 0 && speedB == 0 && chime.isPlaying()) return;
  if ((speedA != 0 || speedB != 0) && chime.isPlaying()) chime.clear();

  driveSystem.setTarget(speedA, speedB);
}

void onControllerLost() {
  stopAllMotors();
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
  motorA.stop();
  motorB.stop();
  setupUploadSafety(&motorA, &motorB, &chime, LED_A, LED_B);

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
  loopUploadSafety();

  // Advance any playing chime notes
  // 电机和旋律都是非阻塞的，必须在 loop() 里不断 update。
  if (!isUploadSafetyMode()) {
    motorA.update();
    motorB.update();
    chime.update();
    if (!chime.isPlaying()) {
      driveSystem.update();
    }
  }

  // 处理网页请求、WebSocket 消息和串口连接提示。
  loopBoatController();
}

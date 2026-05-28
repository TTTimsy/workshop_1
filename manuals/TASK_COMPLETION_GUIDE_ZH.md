# InnoX Boat 任务完成手册

这份手册告诉你这个 repo 里的任务是什么、要改哪个文件、代码应该写在哪里，以及如何编译、上传和测试。

## 你要完成什么

这个项目的目标是让 ESP32-C3 控制一艘双电机小船。手机或电脑连接小船 Wi-Fi 后，打开网页遥控器，通过两个滑杆控制左右电机。

你主要需要完成两个任务：

| 任务 | 函数 | 作用 |
|---|---|---|
| Task 1 | `onStartMotors()` | 点击网页 START 后，让电机播放一段启动旋律。 |
| Task 2 | `onMotorCommand()` | 拖动网页滑杆时，控制对应电机前进、后退或停止。 |

推荐顺序：**先做 Task 2，再做 Task 1**。因为小船最重要的是先能动，再添加启动音效。

## 要改哪个文件

只需要改这个文件：

```text
src/main.cpp
```

不要改这些文件，除非你已经很清楚它们在做什么：

```text
src/motor.h
src/chime.h
src/html.h
src/boat_controller.cpp
src/ap_manager.cpp
```

这些文件负责电机底层、网页、Wi-Fi、WebSocket 等隐藏逻辑。

## Task 2：完成电机控制

先找到 `src/main.cpp` 里的这个函数：

```cpp
void onMotorCommand(char motor, int speed) {
```

网页滑杆每次变化时，系统都会调用它。

参数含义：

| 参数 | 含义 |
|---|---|
| `motor` | 哪个电机，`'a'` 是 Motor A，`'b'` 是 Motor B。 |
| `speed` | 速度，范围是 `-255` 到 `+255`。 |

速度规则：

| speed | 应该做什么 |
|---|---|
| `speed > 0` | 电机正转，船向前推。 |
| `speed < 0` | 电机反转，船向后推。 |
| `speed == 0` | 电机停止。 |

把 `onMotorCommand()` 改成下面这样：

```cpp
void onMotorCommand(char motor, int speed) {
  Motor *m   = (motor == 'b') ? &motorB : &motorA;
  int ledPin = (motor == 'b') ? LED_B  : LED_A;

  if (speed > 0) {
    m->forward(speed);
    digitalWrite(ledPin, HIGH);
    Serial.printf("Motor %c -> FWD  speed=%d\n", motor, speed);
  } else if (speed < 0) {
    m->backward(-speed);
    digitalWrite(ledPin, HIGH);
    Serial.printf("Motor %c -> BWD  speed=%d\n", motor, -speed);
  } else {
    m->stop();
    digitalWrite(ledPin, LOW);
    Serial.printf("Motor %c -> STOP\n", motor);
  }
}
```

重点解释：

```cpp
Motor *m = (motor == 'b') ? &motorB : &motorA;
```

这行根据网页传来的 `motor` 选择电机。如果是 `'b'`，就控制 `motorB`；否则控制 `motorA`。

```cpp
if (speed > 0) {
  m->forward(speed);
}
```

速度是正数时，电机前进。

```cpp
} else if (speed < 0) {
  m->backward(-speed);
}
```

速度是负数时，电机后退。注意这里要写 `-speed`。例如网页传来 `-200`，`backward()` 需要的是正数 `200`。

```cpp
} else {
  m->stop();
}
```

速度是 0 时，停止电机。

## Task 1：完成启动旋律

找到 `src/main.cpp` 里的这个函数：

```cpp
void onStartMotors() {
```

当你在网页上点击 START 按钮时，系统会调用它。

最简单版本：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();

  Serial.println("[Motor] Chime started - motors enabled");
}
```

这段代码会让 Motor A 依次播放 C4、E4、G4 三个音。

参数解释：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
```

| 参数 | 含义 |
|---|---|
| `&motorA` | 用哪个电机播放音符。 |
| `NOTE_C4` | 播放哪个音。 |
| `80` | PWM duty，可以理解成音量/力度，建议先用 60 到 100。 |
| `200` | 持续时间，单位毫秒。 |

必须调用：

```cpp
chime.play();
```

否则音符只是放进队列，不会真正开始播放。

## 进阶版启动旋律：两个电机同时发声

如果想让两个电机一起发声，可以用 `startStep()`、`addToStep()`、`endStep()`。

把 `onStartMotors()` 写成：

```cpp
void onStartMotors() {
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C4, 80);
  chime.addToStep(&motorB, NOTE_E4, 60);
  chime.endStep(250);

  chime.startStep();
  chime.addToStep(&motorA, NOTE_E4, 80);
  chime.addToStep(&motorB, NOTE_G4, 60);
  chime.endStep(250);

  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 90);
  chime.addToStep(&motorB, NOTE_C5, 70);
  chime.endStep(450);

  chime.play();

  Serial.println("[Motor] Chime started - motors enabled");
}
```

解释：

```cpp
chime.startStep();
```

开始定义一拍。

```cpp
chime.addToStep(&motorA, NOTE_C4, 80);
chime.addToStep(&motorB, NOTE_E4, 60);
```

这一拍里，Motor A 和 Motor B 同时播放两个不同的音。

```cpp
chime.endStep(250);
```

这一拍持续 250 毫秒。

## 完整推荐代码

你可以把 `src/main.cpp` 里的 `onStartMotors()` 和 `onMotorCommand()` 改成下面这两个完整版本。

```cpp
void onStartMotors() {
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C4, 80);
  chime.addToStep(&motorB, NOTE_E4, 60);
  chime.endStep(250);

  chime.startStep();
  chime.addToStep(&motorA, NOTE_E4, 80);
  chime.addToStep(&motorB, NOTE_G4, 60);
  chime.endStep(250);

  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 90);
  chime.addToStep(&motorB, NOTE_C5, 70);
  chime.endStep(450);

  chime.play();

  Serial.println("[Motor] Chime started - motors enabled");
}

void onMotorCommand(char motor, int speed) {
  Motor *m   = (motor == 'b') ? &motorB : &motorA;
  int ledPin = (motor == 'b') ? LED_B  : LED_A;

  if (speed > 0) {
    m->forward(speed);
    digitalWrite(ledPin, HIGH);
    Serial.printf("Motor %c -> FWD  speed=%d\n", motor, speed);
  } else if (speed < 0) {
    m->backward(-speed);
    digitalWrite(ledPin, HIGH);
    Serial.printf("Motor %c -> BWD  speed=%d\n", motor, -speed);
  } else {
    m->stop();
    digitalWrite(ledPin, LOW);
    Serial.printf("Motor %c -> STOP\n", motor);
  }
}
```

## 编译

在项目根目录运行：

```powershell
pio run
```

如果看到类似下面的结果，说明编译成功：

```text
[SUCCESS]
```

## 上传到开发板

先用 USB 线连接开发板，然后运行：

```powershell
pio run -t upload
```

如果 PlatformIO 找不到串口，可以指定端口：

```powershell
pio run -t upload --upload-port COM5
```

把 `COM5` 换成你电脑上的真实端口。

Windows 查看串口的方法：

1. 按 `Windows + X`。
2. 打开 Device Manager。
3. 展开 `Ports (COM & LPT)`。
4. 找到类似 `USB Serial Device (COM5)` 的项目。

## 打开串口监视器

上传后运行：

```powershell
pio device monitor --baud 115200
```

你可以在这里看到：

```text
INNOX BOAT CONTROLLER
InnoX Boat ready at 192.168.4.1
```

拖动滑杆时，也会看到：

```text
Motor a -> FWD  speed=120
Motor a -> STOP
Motor b -> BWD  speed=180
```

## 网页测试

1. 连接小船 Wi-Fi。
2. Wi-Fi 名称类似：

```text
InnoX-Boat-XXXX
```

3. 密码：

```text
innox1234
```

4. 浏览器打开：

```text
http://192.168.4.1
```

5. 点击 `START MOTORS`。
6. 听启动旋律。
7. 拖动 Motor A 和 Motor B 的滑杆。

## 测试标准

完成后应该满足：

| 检查项 | 预期结果 |
|---|---|
| 编译 | `pio run` 成功。 |
| 上传 | 固件能上传到 ESP32-C3。 |
| Wi-Fi | 能看到 `InnoX-Boat-XXXX` 热点。 |
| 网页 | 浏览器能打开 `http://192.168.4.1`。 |
| START | 点击后能听到启动旋律。 |
| Motor A 滑杆 | 能控制 A 电机前进、后退、停止。 |
| Motor B 滑杆 | 能控制 B 电机前进、后退、停止。 |
| LED | 电机运行时对应 LED 亮，停止时 LED 灭。 |
| 串口日志 | 能看到 FWD、BWD、STOP 日志。 |

## 常见错误

### 忘记 `chime.play()`

错误：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
```

这样只添加音符，不会播放。

正确：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
chime.play();
```

### 反转时忘记加负号

错误：

```cpp
m->backward(speed);
```

如果 `speed` 是 `-200`，这就把负数传给了 `backward()`。

正确：

```cpp
m->backward(-speed);
```

### 把 `=` 写成 `==`

判断时要用 `==`，赋值时用 `=`。

```cpp
if (speed == 0) {
  m->stop();
}
```

### 音符名字写错

正确：

```cpp
NOTE_C4
NOTE_CS4
NOTE_D4
```

错误：

```cpp
NOTE_C_4
NOTE_C#4
```

## Bonus：播放 MIDI 歌曲

项目里有工具可以把 MIDI 文件转换成 `src/song.h`。

示例：

```powershell
python tools/midi_to_chime.py tools/kdh-golden.mid
```

然后在 `src/main.cpp` 的 `setup()` 里找到 MIDI 彩蛋代码，根据注释取消对应代码。

大概是：

```cpp
#include "song.h"
Motor* trackMap[] = { &motorA, &motorB };
chime.loadSong(&songData, trackMap);
chime.play();
```

注意：这部分是 Bonus，不影响两个核心任务。

## 最小完成版本

如果你只想先过关，最小完成目标是：

1. `onMotorCommand()` 能让电机前进、后退、停止。
2. `onStartMotors()` 能播放至少一个音符。
3. `pio run` 编译成功。
4. 上传后网页能控制小船。

完成这四点，这个 repo 的核心任务就完成了。

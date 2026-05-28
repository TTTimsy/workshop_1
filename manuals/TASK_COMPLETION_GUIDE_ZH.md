# InnoX Boat 任务完成手册

这份手册是根据当前仓库里的 `README.md`、`src/main.cpp`、`src/boat_controller.*`、`src/motor.h`、`src/chime.h`、`src/html.h` 和 `tools/midi_to_chime.py` 重新整理的。目标是把这个 repo 里所有和学生有关的任务、完成方案、需要写的代码、测试方法集中放在一个地方。

## 任务总览

当前仓库里真正需要你完成的核心任务有两个：

| 类型 | 任务 | 需要改哪里 | 是否必做 |
|---|---|---|---|
| Task 2 | 实现网页滑杆控制电机前进、后退、停止 | `src/main.cpp` 的 `onMotorCommand()` | 必做 |
| Task 1 | 实现点击 START 后播放启动旋律 | `src/main.cpp` 的 `onStartMotors()` | 必做 |
| Bonus | 用 MIDI 文件生成电机歌曲 | `tools/midi_to_chime.py` + `src/main.cpp` 的 MIDI 注释块 | 可选 |
| Optional | 改 Wi-Fi 密码 | `src/boat_controller.cpp` | 可选，不建议新手先改 |

推荐完成顺序：

1. 先完成 **Task 2：电机控制**。
2. 再完成 **Task 1：启动旋律**。
3. 最后做编译、上传、网页测试。
4. 有时间再做 MIDI Bonus。

为什么先做 Task 2？因为小船首先要能安全地前进、后退和停止。启动旋律只是锦上添花。

## 已写入但仍保持注释状态的任务代码位置

我已经把 Task 1 和 Task 2 的参考答案写进 `src/main.cpp`，但它们全部保持注释状态，也就是每一行前面都有 `//`。这样做的效果是：

1. 代码答案已经在正确位置，方便你阅读和取消注释。
2. 当前固件行为不会被改变。
3. 烧录时这些任务答案不会执行，因为它们只是注释。

所有被注释掉的任务代码位置如下：

| 任务 | 文件 | 注释块行号 | 真正需要取消注释的代码行 | 说明 |
|---|---|---:|---:|---|
| Task 1 启动旋律 | `src/main.cpp` | 29-44 | 31-43 | 点击 START 后播放三段双电机和声旋律。 |
| Task 2 前进分支 | `src/main.cpp` | 67-71 | 68-70 | `speed > 0` 时让电机前进、点亮 LED、打印 FWD 日志。 |
| Task 2 后退分支 | `src/main.cpp` | 75-79 | 76-78 | `speed < 0` 时让电机后退、点亮 LED、打印 BWD 日志。 |
| Task 2 停止分支 | `src/main.cpp` | 83-87 | 84-86 | `speed == 0` 时停止电机、熄灭 LED、打印 STOP 日志。 |
| Bonus MIDI 歌曲 | `src/main.cpp` | 153-161 | 158-161 | 这是仓库原本已有的 MIDI 彩蛋注释代码，不属于 Task 1/2 必做内容。 |

如果你要真正完成并启用任务，就打开 `src/main.cpp`，只取消上表“真正需要取消注释的代码行”前面的 `//`。标记行例如 `=== TASK 1 SOLUTION CODE - COMMENTED OUT ===` 可以继续保持注释。

## 不需要你改的文件

这些文件是底层支持代码，一般只读不改：

| 文件 | 作用 |
|---|---|
| `src/motor.h` | 提供 `Motor` 类，已经写好了 `forward()`、`backward()`、`stop()`。 |
| `src/chime.h` | 提供 `ChimePlayer` 和 `NOTE_C4` 等音符常量。 |
| `src/html.h` | 保存网页遥控器，网页会发送 START 和滑杆速度。 |
| `src/boat_controller.cpp` | 负责 Wi-Fi、HTTP、WebSocket、JSON 解析和调用你的函数。 |
| `src/ap_manager.cpp` | 负责创建小船 Wi-Fi 热点。 |
| `tools/midi_to_chime.py` | MIDI 转换工具，只在 Bonus 时使用。 |

你主要改：

```text
src/main.cpp
```

## 网页到代码的调用关系

网页里有三个重要操作：

1. 点击 START。
2. 拖动 Motor A 滑杆。
3. 拖动 Motor B 滑杆。

对应到代码：

```text
网页 START 按钮
  -> WebSocket 发送 {"cmd":"start"}
  -> boat_controller.cpp 收到
  -> 调用 onStartMotors()

网页 Motor A / Motor B 滑杆
  -> WebSocket 发送 {"motor":"a","speed":120}
  -> boat_controller.cpp 收到
  -> 调用 onMotorCommand('a', 120)
```

所以你不用管 Wi-Fi 和网页怎么写，只需要写好这两个函数：

```cpp
void onStartMotors() {
}

void onMotorCommand(char motor, int speed) {
}
```

## Task 2：实现电机控制

### 任务目标

当你拖动网页上的滑杆时，电机应该按照滑杆速度工作：

| speed 值 | 含义 | 你要调用 |
|---|---|---|
| `speed > 0` | 前进 | `m->forward(speed)` |
| `speed < 0` | 后退 | `m->backward(-speed)` |
| `speed == 0` | 停止 | `m->stop()` |

速度范围由 `boat_controller.cpp` 限制到：

```text
-255 到 +255
```

### 为什么反转要写 `-speed`

网页发来的后退速度是负数，例如：

```text
speed = -200
```

但是 `backward()` 需要的是正数 PWM 值，所以要写：

```cpp
m->backward(-speed);
```

这样 `-(-200)` 就会变成 `200`。

### 完成代码

请在 `src/main.cpp` 找到 `onMotorCommand()`，把函数内容改成：

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

### 每一段代码的作用

```cpp
Motor *m = (motor == 'b') ? &motorB : &motorA;
```

选择要控制哪个电机。网页发来 `'b'` 就控制 `motorB`，否则控制 `motorA`。

```cpp
int ledPin = (motor == 'b') ? LED_B : LED_A;
```

选择对应 LED。电机运行时 LED 亮，停止时 LED 灭。

```cpp
if (speed > 0) {
  m->forward(speed);
}
```

正数速度表示前进。

```cpp
} else if (speed < 0) {
  m->backward(-speed);
}
```

负数速度表示后退，但传给 `backward()` 的速度要变成正数。

```cpp
} else {
  m->stop();
}
```

速度为 0 时停止。

### Task 2 验收标准

| 操作 | 预期结果 |
|---|---|
| 点击 START 后向上拖 Motor A | A 电机前进方向转动，LED_A 亮。 |
| 松开 Motor A | A 电机停止，LED_A 灭。 |
| 向下拖 Motor A | A 电机反向转动，LED_A 亮。 |
| Motor B 做同样操作 | B 电机响应，LED_B 对应亮灭。 |
| 打开串口监视器 | 能看到 `FWD`、`BWD`、`STOP` 日志。 |

## Task 1：实现 START 启动旋律

### 任务目标

当网页点击 START 后，系统会调用：

```cpp
void onStartMotors() {
}
```

你要在这个函数里添加 `chime` 旋律代码，让电机播放短音效。

### 最小完成版：播放一个音

如果你只想先确认功能，可以写：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 300);
  chime.play();

  Serial.println("[Motor] Chime started - motors enabled");
}
```

这会让 Motor A 播放一个 C4 音，持续 300 毫秒。

### 推荐完成版：三个音组成启动提示

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();

  Serial.println("[Motor] Chime started - motors enabled");
}
```

这会依次播放：

```text
C4 -> E4 -> G4
```

### 进阶完成版：两个电机同时和声

如果想让两个电机同时发声，使用 `startStep()`、`addToStep()`、`endStep()`：

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

### `chime.add()` 参数说明

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
```

| 参数 | 含义 |
|---|---|
| `&motorA` | 哪个电机发声，也可以写 `&motorB`。 |
| `NOTE_C4` | 音符频率，定义在 `src/chime.h`。 |
| `80` | duty，可以理解成声音力度，建议先用 60 到 100。 |
| `200` | 持续时间，单位毫秒。 |

### `startStep()` 这一组函数说明

```cpp
chime.startStep();
chime.addToStep(&motorA, NOTE_C4, 80);
chime.addToStep(&motorB, NOTE_E4, 60);
chime.endStep(250);
```

这表示“一拍”：

```text
Motor A 播放 C4
Motor B 播放 E4
两个音同时开始
持续 250 毫秒
```

### 必须调用 `chime.play()`

下面这样是不够的：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
```

这只是把音符放进队列。必须再写：

```cpp
chime.play();
```

否则不会播放。

### Task 1 验收标准

| 操作 | 预期结果 |
|---|---|
| 打开网页后点击 START | 能听到电机发出短旋律。 |
| START 后按钮变成 MOTORS ACTIVE | 网页已收到启用状态。 |
| 再拖动滑杆 | 电机可以被控制。 |
| 串口监视器 | 能看到 `Chime started` 日志。 |

## 推荐最终代码

如果你希望一次写完两个核心任务，可以把 `src/main.cpp` 中的两个函数改成下面这样。

### `onStartMotors()`

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

### `onMotorCommand()`

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

## 可选子任务：给启动旋律加 LED 闪烁

README 里还给了一个小练习：启动旋律播放时闪 LED。

可以写：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();

  digitalWrite(LED_A, HIGH);
  delay(150);
  digitalWrite(LED_A, LOW);
  delay(80);
  digitalWrite(LED_B, HIGH);
  delay(150);
  digitalWrite(LED_B, LOW);

  Serial.println("[Motor] Chime started - motors enabled");
}
```

注意：短 `delay()` 可以接受，但不要写很长的 `delay()`。长时间阻塞会让 Wi-Fi 和网页响应变差。

## Bonus：播放 MIDI 歌曲

这个 repo 里还有一个可选彩蛋：把 MIDI 文件转换成电机可以播放的歌曲。

相关文件：

```text
tools/midi_to_chime.py
tools/kdh-golden.mid
src/chime.h
src/main.cpp
```

### Step 1：生成 `src/song.h`

在项目根目录运行：

```powershell
python tools/midi_to_chime.py tools/kdh-golden.mid
```

默认会生成：

```text
src/song.h
```

也可以指定速度：

```powershell
python tools/midi_to_chime.py tools/kdh-golden.mid --tempo 140
```

也可以指定输出文件：

```powershell
python tools/midi_to_chime.py tools/kdh-golden.mid --output src/song.h
```

### Step 2：在 `main.cpp` 中启用歌曲

`setup()` 里有一段被注释掉的 MIDI 彩蛋：

```cpp
// #include "song.h"
// Motor* trackMap[] = { &motorA, &motorB };
// chime.loadSong(&songData, trackMap);
// chime.play();
```

实际使用时，`#include "song.h"` 最好放到文件顶部，也就是和这些 include 放在一起：

```cpp
#include "boat_controller.h"
#include "motor.h"
#include "chime.h"
#include "song.h"
```

然后在 `setup()` 里选择合适位置播放：

```cpp
Motor* trackMap[] = { &motorA, &motorB };
chime.loadSong(&songData, trackMap);
chime.play();
```

如果你只是完成核心任务，不需要做这个 Bonus。

### MIDI 注意事项

当前 `src/song.h` 在 `.gitignore` 里被忽略，这是正常的。它是生成文件，可以随时重新生成。

`tools/midi_to_chime.py` 会把 MIDI 轨道映射到 `trackMap`：

```cpp
Motor* trackMap[] = { &motorA, &motorB };
```

其中 track 0 对应 `motorA`，track 1 对应 `motorB`。

## Optional：修改 Wi-Fi 密码

默认 Wi-Fi 密码在 `src/boat_controller.cpp`：

```cpp
apManager.setPassword("innox1234");
```

如果老师要求修改密码，可以改成：

```cpp
apManager.setPassword("yourpassword");
```

新手不建议先改这个。先保证能连接默认热点：

```text
SSID: InnoX-Boat-XXXX
Password: innox1234
```

## 编译

每次改完 `src/main.cpp`，先编译：

```powershell
pio run
```

成功时会看到：

```text
[SUCCESS]
```

如果失败，先看第一条 `error:`，通常是：

| 错误类型 | 常见原因 |
|---|---|
| `expected ';'` | 少了分号。 |
| `was not declared` | 名字写错，例如 `NOTE_C_4`。 |
| `expected '}'` | 少了右花括号。 |
| `invalid conversion` | 参数类型传错。 |

## 上传

用 USB 连接开发板，然后运行：

```powershell
pio run -t upload
```

如果找不到端口，先在 Windows Device Manager 里找 COM 号，然后指定：

```powershell
pio run -t upload --upload-port COM5
```

把 `COM5` 换成你自己的端口。

## 串口监视器

上传后打开串口监视器：

```powershell
pio device monitor --baud 115200
```

你应该看到类似：

```text
INNOX BOAT CONTROLLER
InnoX Boat ready at 192.168.4.1
Open the web page and tap START to enable motors.
```

拖动滑杆时应该看到：

```text
Motor a -> FWD  speed=120
Motor a -> STOP
Motor b -> BWD  speed=180
```

## 网页测试流程

1. 给板子供电。
2. 手机或电脑连接 Wi-Fi：

```text
InnoX-Boat-XXXX
```

3. 输入密码：

```text
innox1234
```

4. 打开浏览器：

```text
http://192.168.4.1
```

5. 点击 `START MOTORS`。
6. 听启动旋律。
7. 轻轻拖动 Motor A 和 Motor B 滑杆。
8. 松开滑杆，确认电机停止。

## 最终验收清单

| 检查项 | 通过标准 |
|---|---|
| 编译 | `pio run` 成功。 |
| 上传 | 固件能上传到 ESP32-C3。 |
| Wi-Fi | 能看到 `InnoX-Boat-XXXX` 热点。 |
| 网页 | 能打开 `http://192.168.4.1`。 |
| START | 点击后能听到启动旋律。 |
| Motor A | 滑杆能控制 A 电机前进、后退、停止。 |
| Motor B | 滑杆能控制 B 电机前进、后退、停止。 |
| LED | 电机运行时对应 LED 亮，停止时 LED 灭。 |
| 串口日志 | 能看到 FWD、BWD、STOP。 |
| 安全 | 松开滑杆后电机必须停止。 |

## 最小完成标准

如果时间很紧，至少做到这四点：

1. `onMotorCommand()` 里实现 `forward()`、`backward()`、`stop()`。
2. `onStartMotors()` 里至少播放一个音符，并调用 `chime.play()`。
3. `pio run` 编译成功。
4. 上传后网页能控制两个电机。

做到这四点，这个 repo 的核心任务就完成了。

## 常见错误

### 1. 忘记 `chime.play()`

错误：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
```

正确：

```cpp
chime.add(&motorA, NOTE_C4, 80, 200);
chime.play();
```

### 2. 反转时忘记负号

错误：

```cpp
m->backward(speed);
```

正确：

```cpp
m->backward(-speed);
```

### 3. 把 `==` 写成 `=`

错误：

```cpp
if (speed = 0) {
  m->stop();
}
```

正确：

```cpp
if (speed == 0) {
  m->stop();
}
```

### 4. 音符名写错

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

### 5. 字符写成字符串

`motor` 是 `char`，所以应该用单引号：

```cpp
if (motor == 'b') {
}
```

不要写：

```cpp
if (motor == "b") {
}
```

## 文件里的任务来源

| 来源 | 找到的任务 |
|---|---|
| `README.md` | Task 1 启动旋律、Task 2 电机控制、MIDI Bonus、编译上传测试流程。 |
| `src/main.cpp` | 明确标注了 Task 1 和 Task 2，另外有 MIDI song easter egg 注释。 |
| `src/boat_controller.cpp` | 说明 START 前油门会被忽略，START 后才调用你的电机控制函数；也显示默认密码。 |
| `src/motor.h` | 提供你要调用的 `forward()`、`backward()`、`stop()`。 |
| `src/chime.h` | 提供你要调用的 `chime.add()`、`chime.startStep()`、`chime.addToStep()`、`chime.endStep()`、`chime.play()`。 |
| `src/html.h` | 网页会发送 START 和滑杆速度，不需要你改。 |
| `tools/midi_to_chime.py` | Bonus：把 MIDI 转为 `src/song.h`。 |

# InnoX Boat — 营前活动

欢迎参加 **InnoX Boat** 嵌入式系统营前活动！你将给一块 ESP32-C3 微控制器编程，让它通过 Wi-Fi 驱动一艘双电机遥控船。

---

## 快速开始

### 1. 连接开发板

用 USB 线把小船控制器插到你的电脑上。

### 2. 找到你的开发板 COM 端口号

这是学生最容易卡住的第一个问题。下面是查找方法：

**在 Windows 上，最常见：**

1. 按 **Windows + X**，然后选择 **Device Manager**。
2. 展开 **Ports (COM & LPT)** 这一栏。
3. 找类似 **"USB Serial Device (COM5)"** 或 **"CP2102N USB to UART Bridge (COM7)"** 的设备。
4. 那个 **COM 编号**，例如 `COM5`、`COM7`、`COM12`，就是你的开发板端口。
5. 如果看不到它：试试换一根 USB 线，有些线只能充电不能传数据，或者安装 CP2102N 驱动。

**在 macOS 上：**

端口看起来会像 `/dev/cu.usbserial-XXXXXXXX` 或 `/dev/tty.usbserial-XXXXXXXX`。
在 Terminal 里运行 `ls /dev/cu.usb*` 或 `ls /dev/tty.usb*` 来查找。

**在 Linux 上：**

端口看起来会像 `/dev/ttyUSB0` 或 `/dev/ttyACM0`。运行 `ls /dev/ttyUSB* /dev/ttyACM*` 查看。

### 3. 打开项目

在已经安装 **PlatformIO** 的 VS Code 中打开这个文件夹。

### 4. 构建并上传，也就是把你的代码发送到开发板

**选项 A — 从 VS Code 上传，推荐：**

- 点击底部 PlatformIO 蓝色状态栏里的 **→ 箭头** 图标。
- 或者按 Ctrl+Shift+P，输入 "PlatformIO: Upload"，然后按 Enter。
- PlatformIO 通常会自动检测端口；如果它询问端口，就选择你的 COM 端口。

**选项 B — 从终端上传，如果 VS Code 自动检测失败：**

把 `COM5` 替换成 **你自己的** COM 端口号，也就是第 2 步找到的端口：

```bash
pio run --target upload --upload-port COM5
```

在 macOS/Linux 上，用 `/dev/cu.usbserial-XXX` 或 `/dev/ttyUSB0` 替代 `COM5`。

**如果上传失败：**

- "Failed to connect" → 检查 Device Manager 里的 COM 端口号。
- "Access denied" → 关闭其他可能正在占用该端口的程序，例如 Arduino IDE。
- 仍然卡住？→ 换一根 USB 线，或者上传时按住开发板上的 **B (RESET)** 按钮。

### 5. 打开串口监视器，用来看调试信息

```bash
pio device monitor --port COM5 --baud 115200
```

把 `COM5` 替换成你的 COM 端口，也就是第 2 步找到的同一个编号。

> **提示：** 如果你看到乱码，例如 `��␀␀␀`，说明波特率不对。请确认 `--baud 115200` 和 `platformio.ini` 里的 `monitor_speed` 一致。

### 6. 连接到小船

加入 Wi-Fi 网络 **`InnoX-Boat-XXXX`**，其中 XXXX 对每块开发板都是唯一的。
Wi-Fi 密码是 **`innox1234`**。
打开浏览器并访问 **http://192.168.4.1**。

> **安全提示：** 小船的 Wi-Fi 默认有密码保护。
> 如果连接断开，网页会显示一个 **Connection Lost** 全屏遮罩，
> 并自动尝试重新连接。点击 **Retry Now** 可以立刻尝试重新连接。

### 7. 点击 START

你会听到电机发出一段提示音，然后就可以使用油门滑杆控制它们。

---

## 项目结构

| 文件 | 你应该怎么处理它 |
|------|-------------------|
| `src/main.cpp` | **这是你写所有代码的地方** |
| `src/motor.h` | 参考文件，`Motor` 类在这里，阅读但不要编辑 |
| `src/chime.h` | 参考文件，`ChimePlayer` 和音符定义在这里，阅读但不要编辑 |
| `src/html.h` | 网页，包含滑杆、断连遮罩、重连逻辑，不需要编辑 |
| everything else | 隐藏的“管道代码”，包括 Wi-Fi、网页服务器、心跳、pong 处理等，不要动 |

### 网页功能

- **大滑杆控制** — 适合触摸操作的大油门滑杆，尺寸会填满你的屏幕。
- **连接恢复能力** — 如果信号断开，会出现一个全屏 **Connection Lost** 遮罩；网页会用指数退避自动重连，时间从 1 秒到 30 秒。
- **有密码保护的 AP** — 小船 Wi-Fi 默认需要 **`innox1234`**。

---

## 总览 — 所有部分如何连接

```text
   网页                     小船 (ESP32-C3)
┌─────────────┐         ┌──────────────────┐
│ Throttle    │  ──WS──→│ onMotorCommand() │──→ motorA.forward(200)
│ Slider A    │         │  你来写这里       │
│             │         └──────────────────┘
│ Throttle    │         ┌──────────────────┐
│ Slider B    │  ──WS──→│ onMotorCommand() │──→ motorB.backward(150)
│             │         │  你来写这里       │
│             │         └──────────────────┘
│ START btn   │  ──WS──→│ onStartMotors()  │──→ chime.add(&motorA, ...)
│             │         │  你来写这里       │    chime.play()
└─────────────┘         └──────────────────┘
```

- **WS** = WebSocket，也就是浏览器和小船之间的实时连接。
- 你要写 **两个函数**。系统会自动调用它们。
- **不用担心** Wi-Fi、网页服务器或 WebSocket，这些都已经隐藏好了。

---

# Task 1 — `onStartMotors()`，播放一段旋律

当用户点击 **START** 按钮时，小船会调用：

```cpp
void onStartMotors() {
  // 你的 chime 代码写在这里
}
```

你的任务：使用 `ChimePlayer` 让电机播放一段短曲。

### Step 1 — 播放一个音符

你能做的最简单事情，就是在一个电机上播放一个音符：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 300);   // motorA, C4, duty=80, 300ms
  chime.play();                           // 开始播放！
}
```

| 参数 | 含义 |
|-----------|---------|
| `&motorA` | 使用哪个电机，`&motorA` 或 `&motorB` |
| `NOTE_C4` | 哪个音符，也就是频率。见下面的音符表。 |
| `80` | 响度，0 = 静音，255 = 最大。建议从 60 到 100 开始。 |
| `300` | 持续时间，单位毫秒。1000 = 1 秒。 |

试试看！修改音符、电机或持续时间。构建、上传，然后点击 START。

### Step 2 — 依次播放多个音符

只要在 `chime.play()` 前面添加更多音符即可：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();
}
```

这会播放：**C4 → E4 → G4**，也就是 C 大调琶音，听起来像一段号角声。

### Step 3 — 同时播放两个电机，和声！

如果想同时播放两个电机，使用 step：

```cpp
void onStartMotors() {
  chime.startStep();                               // 开始一个“和弦”
  chime.addToStep(&motorA, NOTE_C4, 80);            // 旋律
  chime.addToStep(&motorB, NOTE_E4, 60);            // 和声，更轻一点
  chime.endStep(400);                               // 两个音都播放 400 ms

  chime.startStep();
  chime.addToStep(&motorA, NOTE_G4, 80);
  chime.addToStep(&motorB, NOTE_C5, 60);
  chime.endStep(400);

  chime.play();
}
```

**为什么用 `addToStep` 而不是 `add`？**  
`chime.add()` 一次排队一个音符，也就是按顺序播放。`chime.startStep()` / `addToStep()` / `endStep()` 可以把同时开始的音符分组，也就是同时播放。

### Step 4 — 添加 LED 闪烁

你可以在 chime 播放时让板载 LED 闪烁：

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();

  // LED 闪烁，这些会立即运行，因为 chime 是非阻塞的
  digitalWrite(LED_A, HIGH);
  delay(150);
  digitalWrite(LED_A, LOW);
  delay(80);
  digitalWrite(LED_B, HIGH);
  delay(150);
  digitalWrite(LED_B, LOW);
}
```

### 音符参考表

```text
Octave 3:  C3  CS3 D3  DS3 E3  F3  FS3 G3  GS3 A3  AS3 B3
           131 139 147 156 165 175 185 196 208 220 233 247  Hz

Octave 4:  C4  CS4 D4  DS4 E4  F4  FS4 G4  GS4 A4  AS4 B4
           262 277 294 311 330 349 370 392 415 440 466 494  Hz   ← 中音区

Octave 5:  C5  CS5 D5  DS5 E5  F5  FS5 G5  GS5 A5  AS5 B5
           523 554 587 622 659 698 740 784 831 880 932 988  Hz
```

使用方式：`NOTE_C4`、`NOTE_CS4`、`NOTE_D4`，一直到 `NOTE_B4`。
`CS` = C sharp，也就是 C 升半音；`DS` = D sharp，也就是 D 升半音。这些是钢琴上的黑键。

### 可以尝试的想法

| 要做什么 | 代码片段 |
|-----------|-------------|
| 单个音符 | `chime.add(&motorA, NOTE_C4, 80, 500); chime.play();` |
| 上行 3 个音 | `chime.add(&motorA, NOTE_C4, 80, 200); chime.add(...` |
| 两个电机，和弦 | `chime.startStep(); chime.addToStep(&motorA, NOTE_C4, 80); ...` |
| 旋律 + 和声 | 把旋律放在 motorA，和声放在 motorB |
| 你自己的曲子！ | 查找音符频率，然后构建一个序列 |

---

# Task 2 — `onMotorCommand()`，电机控制

每当你拖动网页上的油门滑杆时，这个函数会被调用：

```cpp
void onMotorCommand(char motor, int speed) {
```

| 参数 | 含义 | 示例 |
|-----------|---------|---------|
| `motor` | 哪个电机，`'a'` 或 `'b'` | `'a'` 表示 motor A |
| `speed` | 油门位置 | `127` = 半速前进，`-200` = 接近全速后退 |

**速度值：**

```text
 -255  ←  -200  ←  -100  ←  0  →  100  →  200  →  +255
全速后退        慢速后退   停止  慢速前进       全速前进
```

### 你需要做什么

把这个速度数字翻译成电机命令：

| Speed | 要调用什么 | 为什么 |
|-------|-------------|-----|
| **positive**，+1 到 +255 | `m->forward(speed)` | 让电机以这个速度正转 |
| **negative**，-1 到 -255 | `m->backward(-speed)` | 让电机反转，注意 `-speed` 会把负数转换成正数 |
| **zero** | `m->stop()` | 停止电机 |

**我们已经为你写好了比较麻烦的指针部分：**

```cpp
Motor *m   = (motor == 'b') ? &motorB : &motorA;   // 已经给好！
int ledPin = (motor == 'b') ? LED_B  : LED_A;      // 已经给好！
```

- `m` 是一个指向正确 `Motor` 对象的指针。使用 `m->forward(...)` 等方法。
- `ledPin` 是这个电机对应的 LED GPIO 引脚。使用 `digitalWrite(ledPin, HIGH)`。

### 分步骤

**Step 1 — 只做前进**

先从简单开始：当 speed 是正数时，让电机前进，否则停止。

```cpp
if (speed > 0) {
  m->forward(speed);
} else {
  m->stop();
}
```

**Step 2 — 加入后退**

```cpp
if (speed > 0) {
  m->forward(speed);
} else if (speed < 0) {
  m->backward(-speed);     // 注意这里的负号！
} else {
  m->stop();
}
```

**Step 3 — 加入 LED**

```cpp
if (speed > 0) {
  m->forward(speed);
  digitalWrite(ledPin, HIGH);
} else if (speed < 0) {
  m->backward(-speed);
  digitalWrite(ledPin, HIGH);
} else {
  m->stop();
  digitalWrite(ledPin, LOW);
}
```

**Step 4 — 加入日志，方便调试**

```cpp
if (speed > 0) {
  m->forward(speed);
  digitalWrite(ledPin, HIGH);
  Serial.printf("Motor %c → FWD  speed=%d\n", motor, speed);
} else if (speed < 0) {
  m->backward(-speed);
  digitalWrite(ledPin, HIGH);
  Serial.printf("Motor %c → BWD  speed=%d\n", motor, -speed);
} else {
  m->stop();
  digitalWrite(ledPin, LOW);
  Serial.printf("Motor %c → STOP\n", motor);
}
```

打开串口监视器，也就是 Tools → Serial Monitor，115200 baud。拖动滑杆时观察消息出现。

---

### Motor 类参考

`motorA` 和 `motorB` 可以使用这些方法：

| 方法 | 它做什么 |
|--------|-------------|
| `forward(0–255)` | 让电机以给定速度正转 |
| `backward(0–255)` | 让电机以给定速度反转 |
| `stop()` | 立刻停止电机 |
| `getSpeed()` | 返回你上一次设置的速度 |

示例：

```cpp
motorA.forward(200);      // motor A 以 200 前进
motorB.backward(128);     // motor B 以半速后退
motorA.stop();            // 停止 motor A

int s = motorA.getSpeed();   // 读取当前速度
```

---

## 常见错误

| 错误 | 修复 |
|---------|-----|
| `forward(-100)` | `forward()` 需要 0–255。反向请用 `backward(100)`。 |
| `backward(-50)` | `backward()` 需要 0–255。请用 `backward(50)`。 |
| 忘记 `chime.play()` | 音符只是排队，不调用 `chime.play()` 就不会播放。 |
| 在 chime 里使用 `delay()` | `delay()` 会冻结所有东西，Wi-Fi 会断开。请使用 `ChimePlayer`。 |
| 把 `NOTE_C4` 写成 `NOTE_C_4` | 检查 `chime.h` 里的常量准确名称。 |

---

## 测试流程

1. **先做 Task 2** — 用滑杆让电机转起来。它只是 3 个条件和 3 个方法调用。
2. **再做 Task 1** — 当你可以控制电机后，让它们在 START 被按下时播放一段曲子。
3. **经常构建 + 上传** — 小步修改，频繁测试。如果有什么坏了，你会准确知道是哪一步导致的。

---

## Bonus：MIDI 彩蛋

你可以通过小船电机播放任何 MIDI 文件！请查看 `main.cpp` 中 `setup()` 末尾被注释掉的代码块。

---

祝你开船愉快！

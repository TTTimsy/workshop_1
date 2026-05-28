# InnoX Boat 代码中文导读

这份文档配合源码里的中文注释阅读。源码中已经补了关键行注释；这里按文件把“这个 repo 到底做什么”和“每个代码文件负责什么”讲清楚。

## 一句话概括

这个仓库是一个 PlatformIO + Arduino 的 ESP32-C3 遥控船教学项目。ESP32-C3 开一个 Wi-Fi 热点，浏览器连接后打开船上的网页遥控器；网页通过 WebSocket 发送 START 和两个电机的速度，固件再驱动两路直流电机。电机还可以通过 PWM 频率变化播放简单旋律。

## 文件架构表

| 路径 | 类型 | 作用 |
|---|---|---|
| `platformio.ini` | PlatformIO 配置 | 指定开发板、Arduino 框架、依赖库、串口速度和 USB CDC 编译宏。 |
| `.vscode/extensions.json` | VS Code 配置 | 推荐安装 PlatformIO IDE 插件。 |
| `.vscode/settings.json` | VS Code 配置 | 指向 `compile_commands.json`，帮助 IntelliSense 找到 `Arduino.h`、`WiFi.h`。 |
| `compile_commands.json` | 生成文件 | PlatformIO 生成的编译数据库，供编辑器索引用。 |
| `src/main.cpp` | 学生入口 | 定义两个电机和旋律播放器；学生主要补 `onStartMotors()` 和 `onMotorCommand()`。 |
| `src/motor.h` | 电机封装 | 用 ESP32 LEDC PWM 控制一个直流电机正转、反转、停止，也支持按频率播放音符。 |
| `src/chime.h` | 旋律系统 | 定义音符频率、旋律步骤、MIDI 歌曲结构和非阻塞播放器 `ChimePlayer`。 |
| `src/boat_controller.h` | 控制器接口 | 暴露 `setupBoatController()`、`loopBoatController()`，声明学生回调函数。 |
| `src/boat_controller.cpp` | 网络和消息分发 | 启动 Wi-Fi AP、HTTP 服务器、WebSocket 服务器，解析网页命令并调用学生函数。 |
| `src/ap_manager.h` | AP 类声明 | 声明热点管理类 `APManager`。 |
| `src/ap_manager.cpp` | AP 类实现 | 生成唯一 SSID、扫描频道、启动 ESP32 SoftAP。 |
| `src/html.h` | 内嵌网页 | 保存完整 HTML/CSS/JS 字符串，网页有 START 按钮、双油门滑杆和断线重连逻辑。 |
| `tools/midi_to_chime.py` | 工具脚本 | 在电脑上把 `.mid` 文件转换为 `src/song.h`，让电机播放 MIDI 旋律。 |
| `tools/kdh-golden.mid` | 示例素材 | MIDI 示例文件。 |
| `include/`、`lib/`、`test/` | PlatformIO 默认目录 | 当前只有说明文件，不参与核心功能。 |

## 运行流程

1. `setup()` 在 `src/main.cpp` 中启动串口、初始化 LED、调用 `setupBoatController()`。
2. `setupBoatController()` 在 `src/boat_controller.cpp` 中启动 Wi-Fi 热点、HTTP 页面和 WebSocket。
3. 手机或电脑连接 `InnoX-Boat-XXXX`，浏览器访问 `http://192.168.4.1`。
4. ESP32 的 HTTP 服务器把 `src/html.h` 里的 `INDEX_HTML` 返回给浏览器。
5. 网页 JavaScript 建立 `ws://192.168.4.1:81` WebSocket。
6. 用户点 START，网页发送 `{"cmd":"start"}`。
7. 固件收到 START 后设置 `motorsEnabled = true`，调用 `onStartMotors()`。
8. 用户拖动 A/B 滑杆，网页发送 `{"motor":"a","speed":120}` 这样的 JSON。
9. 固件限制速度到 `-255..255`，调用 `onMotorCommand('a', 120)`。
10. 学生代码根据正负速度调用 `forward()`、`backward()` 或 `stop()`。
11. `loop()` 持续调用 `motorA.update()`、`motorB.update()`、`chime.update()` 和 `loopBoatController()`，保证旋律和网络都不会卡住。

## `src/ap_manager.h` 逐行说明

| 行/区域 | 说明 |
|---|---|
| `#ifndef AP_MANAGER_H` | 头文件保护：如果这个头文件已经被包含过，就避免重复定义。 |
| `#define AP_MANAGER_H` | 和上一行配合，标记这个头文件已经开始被处理。 |
| `#include <Arduino.h>` | 引入 Arduino 基础 API，例如 `Serial`、`delay()`、`IPAddress`。 |
| `#include <WiFi.h>` | 引入 ESP32 Wi-Fi API，例如 `WiFi.softAP()`、`WiFi.scanNetworks()`。 |
| `class APManager` | 声明热点管理类，封装所有 SoftAP 相关操作。 |
| `public:` | 下面的方法可以被外部代码调用。 |
| `APManager()` | 构造函数，负责设置默认模式并生成默认 SSID。 |
| `setSsid()` | 修改热点名称。 |
| `setPassword()` | 修改热点密码，空密码表示开放热点。 |
| `setChannel()` | 手动指定频道；传 0 表示自动选择频道。 |
| `setMode()` | 设置 Wi-Fi 模式，项目中使用 `WIFI_AP`。 |
| `findBestChannel()` | 扫描附近 Wi-Fi 并选一个拥挤程度较低的频道。 |
| `begin()` | 按当前配置真正启动热点。 |
| `getSsid()` | 返回当前热点名称。 |
| `getChannel()` | 返回当前热点频道。 |
| `getIp()` | 返回热点 IP，浏览器通常访问 `192.168.4.1`。 |
| `generateSsidFromMac()` | 静态工具函数，用芯片 MAC 生成唯一 SSID。 |
| `private:` | 下面的字段只能被类自己使用，外部不能直接改。 |
| `_ssid[32]` | 保存 SSID 字符串。 |
| `_password[64]` | 保存 Wi-Fi 密码字符串。 |
| `_channel` | 保存频道，0 代表自动选择。 |
| `_wifiMode` | 保存 Wi-Fi 工作模式。 |
| `#endif` | 结束头文件保护。 |

## 各代码文件重点导读

### `platformio.ini`

`[env:seeed_xiao_esp32c3]` 定义一个构建环境。`platform = espressif32` 选择 ESP32 平台包；`board = seeed_xiao_esp32c3` 选择具体开发板；`framework = arduino` 表示使用 `setup()`/`loop()` 编程模型。`lib_deps` 安装 WebSocket 和 JSON 库。`monitor_speed = 115200` 必须和 `Serial.begin(115200)` 保持一致。

### `src/main.cpp`

这个文件是教学入口。顶部 `#include` 引入控制器、电机、旋律播放器。`Motor motorA(...)` 和 `Motor motorB(...)` 分别绑定 A/B 电机引脚。`ChimePlayer chime` 是全局旋律播放器。

`onStartMotors()` 会在网页 START 按下后执行，适合加入启动音效。当前练习版只打印日志。`onMotorCommand(char motor, int speed)` 会在滑杆变化时执行；`motor` 是 `'a'` 或 `'b'`，`speed` 是 `-255..255`。正数应前进，负数应后退，0 应停止。

`playStartupChime()` 在开机后排入一段固定旋律。`setup()` 初始化串口和 LED，调用 `setupBoatController()` 启动网络服务。`loop()` 不断推进电机、旋律和网络事件。

### `src/motor.h`

`Motor` 类把一个电机抽象成三个常用动作：`forward(speed)`、`backward(speed)`、`stop()`。底层通过两个 PWM 通道控制 H 桥：正转时 channel1 输出 PWM、channel2 为 0；反转时相反；停止时两个通道都为 0。

同一个类还支持 `playNote(freqHz, duty, durationMs)`。它把 PWM 频率改成音符频率，让电机线圈发声。`update()` 会检查音符是否到期，到期后自动关掉 PWM，所以播放旋律不会阻塞 Wi-Fi。

### `src/chime.h`

前半部分定义 `NOTE_C4`、`NOTE_D4` 等音符频率。`ChimeNote` 表示“某个电机播放某个频率和 duty”。`ChimeStep` 表示“一拍”，一拍里可以有多个音符同时播放。`ChimePlayer` 用动态数组保存步骤，然后用 `play()` 启动，用 `update()` 自动推进。

`add()` 是单音简写；`startStep()`、`addToStep()`、`endStep()` 可以做双电机和声。`loadSong()` 可以加载 `tools/midi_to_chime.py` 生成的歌曲数据。

### `src/boat_controller.cpp`

这个文件负责网络。`APManager apManager` 管热点，`WebServer httpServer(80)` 管网页，`WebSocketsServer wsServer(81)` 管实时控制。`motorsEnabled` 是安全开关，START 之前油门消息会被忽略。

`onWsEvent()` 是 WebSocket 回调。浏览器连接时，它同步当前电机启用状态。收到 `{"ping":1}` 时回复 `{"pong":1}`。收到 `{"cmd":"start"}` 时解锁电机并调用 `onStartMotors()`。收到 `{"motor":"a","speed":120}` 时检查电机名、限制速度，然后调用 `onMotorCommand()`。

### `src/ap_manager.cpp`

构造函数设置默认频道和模式，并根据 MAC 生成 SSID。`setSsid()`、`setPassword()`、`setChannel()`、`setMode()` 只是保存配置。`findBestChannel()` 会扫描附近热点数量，选择网络最少的频道。`begin()` 会先设置 Wi-Fi 模式，再自动选择频道，最后调用 `WiFi.softAP()` 启动热点。

### `src/html.h`

这是固件内嵌网页。CSS 定义深色背景、两个竖向滑杆、START 按钮和断线遮罩。JavaScript 负责连接 WebSocket、心跳检测、断线重连、按钮状态和滑杆拖动。

滑杆的核心逻辑是把触摸坐标转换成速度：越靠上越接近 `+255`，越靠下越接近 `-255`，中间是 `0`。松手时网页会发送速度 `0`，让电机停止。

### `tools/midi_to_chime.py`

这个脚本解析 MIDI 文件的 `MThd` 和 `MTrk` 块，只保留 Note On/Off 事件。它把音符开始和结束配对，计算持续时间，再把同时开始的音符合并成 `ChimeStep`。最后输出 C++ 头文件，里面有 `_song_steps[]` 和 `songData`，可以被 `ChimePlayer.loadSong()` 加载。

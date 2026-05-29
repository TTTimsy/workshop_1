# 小船平稳运行与灵活转弯设计补充说明

## 1. 为什么参考 DifferentialDrive

这里说的 DifferentialDrive，不是要把小船当作严格的轮式机器人，而是参考成熟双执行器平台的控制架构：

```text
左/右执行器独立输出
  -> 可以直行
  -> 可以差速转弯
  -> 可以一侧前进一侧后退实现小半径转向
```

双马达小船和差速轮式机器人很像：两侧推力差决定转向趋势。不同点是小船在水中会有滑移、惯性和螺旋桨负载变化，所以论文模型不能直接照搬，但它们支持我们采用“左右输出 + 速度/转向混控 + 加速度限制 + 安全超时”的控制结构。

## 2. 文献与工程依据

### 2.1 Kanayama 等人的稳定跟踪控制

Kanayama, Kimura, Miyazaki, Noguchi 在 1990 年 ICRA 论文 *A stable tracking control method for an autonomous mobile robot* 中提出非完整移动机器人稳定跟踪控制。该论文的核心是把目标姿态和参考速度转换为合理的线速度、角速度，并引入速度/加速度限制来避免打滑。

对我们有用的点：

```text
不要让用户输入直接变成马达瞬时输出；
应该经过速度限制、加速度限制和平滑处理。
```

参考：

- https://cir.nii.ac.jp/crid/1573387449091026048
- https://doi.org/10.1109/robot.1990.126006

### 2.2 Differential-drive / unicycle 模型

Doria-Cerezo 等人的 *Sliding mode control of a differential-drive mobile robot following a path* 明确给出了差速机器人的速度关系：

```text
线速度 v 由左右轮速度和决定
角速度 u 由左右轮速度差决定
```

对我们的小船来说，可以类比为：

```text
前进趋势 ~= 左右马达平均推力
转向趋势 ~= 左右马达推力差
```

参考：

- https://upcommons.upc.edu/bitstreams/992a7d05-d152-48b4-9396-69bf95f5e797/download

### 2.3 现代差速机器人仍然采用同类思想

Zhang 等人的 2024 年论文 *Universal Trajectory Optimization Framework for Differential Drive Robot Class* 说明差速驱动机器人广泛存在于服务、救援等场景，且不同机构仍可以抽象成线速度/角速度或左右驱动量的规划问题。

参考：

- https://arxiv.org/abs/2409.07924

Isleyen 等人的 *Feedback Motion Prediction for Safe Unicycle Robot Navigation* 也把 differential drive robots 建模为 kinematic unicycle，用于安全避障和反应式导航。

参考：

- https://arxiv.org/abs/2209.12648

### 2.4 工程实现参考：WPILib DifferentialDrive

FIRST Robotics 的 WPILib `DifferentialDrive` 是成熟工程库，提供三种常见驾驶方式：

```text
Tank Drive      左右两侧独立控制
Arcade Drive    速度 + 转向
Curvature Drive 速度 + 曲率/转弯半径，并支持 quick turn
```

这正好对应我们当前争论：

```text
双拉杆灵活，但直线不稳；
速度+转向稳定，但避障自由度下降；
Curvature / quick turn 可以在两者之间折中。
```

参考：

- https://docs.wpilib.org/en/stable/docs/software/hardware-apis/motors/wpi-drive-classes.html
- https://github.wpilib.org/allwpilib/docs/release/cpp/classfrc_1_1_differential_drive.html

## 3. 我们不应该只保留一种驾驶方式

最终设计不应该简单删掉双拉杆，而是做多模式。

### 模式 A：Tank / 双拉杆模式

用途：

```text
绕障碍
原地/小半径转向
调试左右马达
测试水面推力差
```

输入：

```text
左拉杆 -> A 马达目标速度
右拉杆 -> B 马达目标速度
```

特点：

```text
灵活度最高
学习成本略高
直线保持依赖驾驶者手感
```

### 模式 B：Curvature / 曲率驾驶模式

用途：

```text
正式驾驶
直线前进
稳定大弧线转弯
高速时避免突然甩头
```

输入：

```text
throttle -> 前进/后退速度
turn     -> 转弯大小
```

混控思路：

```cpp
if (quickTurn || abs(throttle) < lowSpeedThreshold) {
  leftTarget  = throttle + turn;
  rightTarget = throttle - turn;
} else {
  leftTarget  = throttle + abs(throttle) * turn;
  rightTarget = throttle - abs(throttle) * turn;
}
```

特点：

```text
高速更稳
转弯半径更可控
仍然可以用 quickTurn 做小半径转向
```

### 模式 C：Dock / 精细操控模式

用途：

```text
靠岸
低速绕障碍
狭窄空间掉头
测试水池边缘操作
```

策略：

```text
限制最大功率
降低加速度
允许左右马达反向
保留更大的转向权限
```

特点：

```text
速度慢
操作细腻
不容易突然冲出去
```

## 4. 底层统一 DriveSystem

无论网页使用哪个模式，固件底层应该统一成同一个控制管线：

```text
网页输入
  -> mode mixer
  -> targetA / targetB
  -> deadzone
  -> expo 曲线
  -> 左右马达 trim
  -> 加速度限制
  -> 方向切换保护
  -> Motor.forward/backward/stop
```

### 4.1 deadzone

忽略中心附近的小抖动：

```cpp
if (abs(input) < DRIVE_DEADZONE) input = 0;
```

### 4.2 expo 曲线

让中间区域更细腻，拉满仍然有力：

```cpp
float x = input / 255.0f;
float y = expo * x * x * x + (1.0f - expo) * x;
output = y * 255;
```

建议初值：

```text
expo = 0.35
```

### 4.3 slew rate / 加速度限制

目标速度不能瞬间变成实际速度：

```cpp
delta = target - current;
delta = constrain(delta, -maxStep, maxStep);
current += delta;
```

建议：

```text
普通加速：每 20ms 增加 8-12
普通减速：每 20ms 减少 14-20
Dock 模式：每 20ms 增加 5-8
```

### 4.4 方向切换保护

前进和后退不能直接硬切：

```text
如果 current > 0 且 target < 0：
  先降到 0
  停 100-150ms
  再反向起步
```

同理：

```text
current < 0 且 target > 0
```

### 4.5 左右马达 trim

船跑偏不一定是驾驶问题，可能是左右马达/螺旋桨推力不一致。

建议配置：

```cpp
static constexpr float MOTOR_A_TRIM = 1.00f;
static constexpr float MOTOR_B_TRIM = 1.00f;
```

如果船总是向右偏：

```text
可能 A 推力大，或 B 推力小
可以略降 A，或略升 B
```

## 5. 网页设计

不要做复杂菜单，保持现场可操作。

建议顶部加一个模式切换：

```text
TANK | CURVE | DOCK
```

### Tank 页面

沿用当前两个竖向拉杆：

```text
Motor A
Motor B
```

### Curve 页面

两个控制：

```text
Throttle 竖向
Turn 横向或圆形摇杆
QuickTurn 按钮/开关
```

### Dock 页面

可以复用 Tank 页面，但限制最大速度：

```text
maxDuty / maxInput 降低
加速度降低
```

## 6. 通信协议

当前 `drive` 包可以保留：

```json
{"cmd":"drive","a":120,"b":100}
```

网页端负责把不同驾驶模式先换算成 A/B 目标速度，再发给固件。

更进一步的版本可以支持：

```json
{"cmd":"drive_mode","mode":"curve","throttle":150,"turn":60,"quickTurn":false}
```

但第一版不建议这么做，因为会增加固件和网页耦合。先让网页统一输出 A/B，更稳。

## 7. 安全策略

保留现有策略：

```text
网页每 150ms 发送当前目标
固件 700ms 没收到 active controller 的 drive 包就停车
只允许 active controller 控制
其他连接 view only
```

新增策略：

```text
切换模式时立即发送 0,0
切换模式后 300ms 内限制加速度
连接断开时清空 target/current
```

## 8. 推荐实施顺序

第一阶段：不改 UI，大幅提升稳定性

```text
新增 DriveSystem
保留双拉杆
加入 deadzone/expo/slew rate/方向切换保护/trim
```

第二阶段：加入模式切换

```text
TANK 模式保留现有双拉杆
CURVE 模式加入 throttle + turn
DOCK 模式低速精细控制
```

第三阶段：水池调参

```text
测 A/B 最小稳定 PWM
测直线跑偏
调 MOTOR_A_TRIM / MOTOR_B_TRIM
调高速转向上限
调加速度
```

## 9. 推荐默认参数

```cpp
static constexpr int DRIVE_UPDATE_MS = 20;
static constexpr float DRIVE_EXPO = 0.35f;

static constexpr int DRIVE_ACCEL_STEP = 10;
static constexpr int DRIVE_DECEL_STEP = 18;
static constexpr int DRIVE_DOCK_ACCEL_STEP = 6;

static constexpr int DIRECTION_REVERSAL_PAUSE_MS = 120;

static constexpr float MOTOR_A_TRIM = 1.00f;
static constexpr float MOTOR_B_TRIM = 1.00f;

static constexpr int CURVE_LOW_SPEED_THRESHOLD = 60;
static constexpr int CURVE_HIGH_SPEED_TURN_LIMIT = 130;
```

## 10. 本项目的设计结论

最终结论：

```text
不要放弃双拉杆。
不要只做 throttle + turn。
采用 DifferentialDrive 思想，但保留 Tank 模式。
底层统一 DriveSystem，所有模式都经过平滑、限加速度、trim 和安全保护。
```

这样可以同时满足：

```text
直线更稳
转弯更顺
避障仍然灵活
靠岸更细
调试更容易
```

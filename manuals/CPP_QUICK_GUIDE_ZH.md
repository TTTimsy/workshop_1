# C++ 快速参考手册

这份手册写给只学过 Python、现在要读写 Arduino / ESP32 C++ 代码的人。重点不是完整学习 C++，而是让你能看懂并完成这个小船项目。

## 1. C++ 和 Python 最大的区别

| Python | C++ |
|---|---|
| 变量不需要声明类型 | 变量通常要写类型 |
| 用缩进表示代码块 | 用 `{}` 表示代码块 |
| 行尾通常不用分号 | 大多数语句行尾要写 `;` |
| 函数用 `def` | 函数要写返回类型 |
| `True` / `False` | `true` / `false` |
| `and` / `or` / `not` | `&&` / `||` / `!` |

Python：

```python
speed = 100
if speed > 0:
    print("forward")
```

C++：

```cpp
int speed = 100;
if (speed > 0) {
  Serial.println("forward");
}
```

## 2. 文件类型

| 文件后缀 | 作用 |
|---|---|
| `.cpp` | C++ 源文件，真正写逻辑的地方。 |
| `.h` | C++ 头文件，通常放类、函数声明、常量定义。 |
| `.ino` | Arduino 项目常见文件，本项目使用 `.cpp`。 |

本项目主要写：

```text
src/main.cpp
```

## 3. 变量

C++ 变量格式：

```cpp
类型 变量名 = 初始值;
```

例子：

```cpp
int speed = 100;
float voltage = 3.3;
bool enabled = true;
char motor = 'a';
```

常见类型：

| 类型 | 含义 | 例子 |
|---|---|---|
| `int` | 整数 | `int speed = 120;` |
| `float` | 小数 | `float v = 3.14;` |
| `double` | 更高精度小数 | `double x = 1.234;` |
| `bool` | 布尔值 | `bool ok = true;` |
| `char` | 单个字符 | `char motor = 'a';` |
| `const char *` | 字符串 | `const char *name = "boat";` |

注意：

```cpp
char c = 'a';        // 单个字符用单引号
const char *s = "a"; // 字符串用双引号
```

## 4. 分号

C++ 大多数语句结尾都要写分号：

```cpp
int speed = 100;
m->forward(speed);
Serial.println("hello");
```

这些地方不要写分号：

```cpp
if (speed > 0) {
}

void setup() {
}
```

## 5. 函数

Python：

```python
def add(a, b):
    return a + b
```

C++：

```cpp
int add(int a, int b) {
  return a + b;
}
```

格式：

```cpp
返回类型 函数名(参数类型 参数名, 参数类型 参数名) {
  函数内容;
}
```

如果函数不返回值，返回类型写 `void`：

```cpp
void sayHello() {
  Serial.println("hello");
}
```

本项目里的例子：

```cpp
void onMotorCommand(char motor, int speed) {
  // 控制电机
}
```

含义：

| 部分 | 含义 |
|---|---|
| `void` | 这个函数没有返回值。 |
| `onMotorCommand` | 函数名。 |
| `char motor` | 第一个参数，单个字符，比如 `'a'` 或 `'b'`。 |
| `int speed` | 第二个参数，整数速度。 |

## 6. if / else 判断

Python：

```python
if speed > 0:
    print("forward")
elif speed < 0:
    print("backward")
else:
    print("stop")
```

C++：

```cpp
if (speed > 0) {
  Serial.println("forward");
} else if (speed < 0) {
  Serial.println("backward");
} else {
  Serial.println("stop");
}
```

常见比较：

```cpp
speed == 0    // 等于
speed != 0    // 不等于
speed > 0     // 大于
speed < 0     // 小于
speed >= 100  // 大于等于
speed <= 100  // 小于等于
```

非常重要：

```cpp
speed = 0;   // 赋值：把 speed 改成 0
speed == 0;  // 判断：speed 是否等于 0
```

## 7. 逻辑运算

Python：

```python
if speed > 0 and enabled:
    ...
```

C++：

```cpp
if (speed > 0 && enabled) {
  ...
}
```

对照表：

| Python | C++ | 含义 |
|---|---|---|
| `and` | `&&` | 并且 |
| `or` | `||` | 或者 |
| `not` | `!` | 取反 |

例子：

```cpp
if (motor == 'a' || motor == 'b') {
  Serial.println("valid motor");
}

if (!enabled) {
  Serial.println("not enabled");
}
```

## 8. for 循环

Python：

```python
for i in range(5):
    print(i)
```

C++：

```cpp
for (int i = 0; i < 5; i++) {
  Serial.println(i);
}
```

解释：

```cpp
for (int i = 0; i < 5; i++)
```

| 部分 | 含义 |
|---|---|
| `int i = 0` | 从 0 开始。 |
| `i < 5` | 只要 i 小于 5，就继续循环。 |
| `i++` | 每次循环结束后 i 加 1。 |

## 9. while 循环

Python：

```python
while speed > 0:
    speed -= 1
```

C++：

```cpp
while (speed > 0) {
  speed -= 1;
}
```

## 10. 注释

单行注释：

```cpp
// 这是单行注释
```

多行注释：

```cpp
/*
  这是多行注释
  可以写很多行
*/
```

## 11. 常量和宏

本项目里经常看到：

```cpp
#define LED_A 20
#define MOTOR_A1 5
```

这表示把名字替换成数字。比如：

```cpp
digitalWrite(LED_A, HIGH);
```

编译时大概会被理解为：

```cpp
digitalWrite(20, HIGH);
```

好处是代码更容易读。

## 12. include

C++ 用 `#include` 引入别的文件或库：

```cpp
#include "motor.h"
#include "chime.h"
#include <Arduino.h>
```

区别：

| 写法 | 通常表示 |
|---|---|
| `"motor.h"` | 引入项目自己的文件。 |
| `<Arduino.h>` | 引入系统/库提供的文件。 |

## 13. 对象和方法

Python：

```python
motor.forward(100)
```

C++：

```cpp
motor.forward(100);
```

这表示调用 `motor` 这个对象的 `forward()` 方法。

本项目里：

```cpp
motorA.forward(200);
motorB.backward(128);
motorA.stop();
```

## 14. 指针和 `->`

你在本项目里会看到：

```cpp
Motor *m = (motor == 'b') ? &motorB : &motorA;
m->forward(speed);
```

先不用深入理解指针，可以这样类比：

```cpp
m->forward(speed);
```

大概等于 Python 里的：

```python
m.forward(speed)
```

为什么不用点号 `.`？

因为 `m` 不是电机对象本身，而是“指向某个电机对象的位置”。C++ 里对指针调用方法用 `->`。

几个符号先这样记：

| 符号 | 简单理解 |
|---|---|
| `Motor *m` | m 是一个指向 Motor 的变量。 |
| `&motorA` | 取 motorA 的地址。 |
| `m->forward()` | 让 m 指向的那个电机 forward。 |

## 15. 三元表达式

本项目里有：

```cpp
Motor *m = (motor == 'b') ? &motorB : &motorA;
```

格式：

```cpp
条件 ? 条件为真时的值 : 条件为假时的值
```

上面那行等价于：

```cpp
Motor *m;

if (motor == 'b') {
  m = &motorB;
} else {
  m = &motorA;
}
```

另一个例子：

```cpp
int ledPin = (motor == 'b') ? LED_B : LED_A;
```

意思是：如果控制的是 B 电机，就用 `LED_B`，否则用 `LED_A`。

## 16. Arduino 常用函数

### `pinMode`

```cpp
pinMode(LED_A, OUTPUT);
```

设置引脚模式。`OUTPUT` 表示这个引脚用来输出电信号。

### `digitalWrite`

```cpp
digitalWrite(LED_A, HIGH);
digitalWrite(LED_A, LOW);
```

给引脚输出高电平或低电平。

| 值 | 含义 |
|---|---|
| `HIGH` | 高电平，通常代表开。 |
| `LOW` | 低电平，通常代表关。 |

### `delay`

```cpp
delay(1000);
```

暂停 1000 毫秒，也就是 1 秒。

注意：在需要 Wi-Fi 保持响应的程序里，不要频繁使用长 `delay()`。

### `Serial.begin`

```cpp
Serial.begin(115200);
```

启动串口，波特率是 115200。

### `Serial.println`

```cpp
Serial.println("hello");
```

在串口监视器里打印一行文字。

### `Serial.printf`

```cpp
Serial.printf("speed=%d\n", speed);
```

格式化打印。

常见格式：

| 格式 | 含义 |
|---|---|
| `%d` | 整数 |
| `%c` | 字符 |
| `%s` | 字符串 |
| `\n` | 换行 |

例子：

```cpp
Serial.printf("Motor %c speed=%d\n", motor, speed);
```

如果 `motor` 是 `'a'`，`speed` 是 `120`，输出：

```text
Motor a speed=120
```

## 17. Arduino 程序结构

普通 Arduino 程序通常有：

```cpp
void setup() {
  // 只运行一次
}

void loop() {
  // 一直重复运行
}
```

`setup()` 用来初始化：

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(LED_A, OUTPUT);
}
```

`loop()` 用来持续工作：

```cpp
void loop() {
  digitalWrite(LED_A, HIGH);
  delay(1000);
  digitalWrite(LED_A, LOW);
  delay(1000);
}
```

本项目里的 `loop()` 会持续处理：

```cpp
motorA.update();
motorB.update();
chime.update();
loopBoatController();
```

## 18. 本项目最重要的代码模板

### 控制电机

```cpp
void onMotorCommand(char motor, int speed) {
  Motor *m   = (motor == 'b') ? &motorB : &motorA;
  int ledPin = (motor == 'b') ? LED_B  : LED_A;

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
}
```

### 播放一个音符

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 300);
  chime.play();
}
```

### 播放多个音符

```cpp
void onStartMotors() {
  chime.add(&motorA, NOTE_C4, 80, 200);
  chime.add(&motorA, NOTE_E4, 80, 200);
  chime.add(&motorA, NOTE_G4, 80, 400);
  chime.play();
}
```

### 两个电机同时播放

```cpp
void onStartMotors() {
  chime.startStep();
  chime.addToStep(&motorA, NOTE_C4, 80);
  chime.addToStep(&motorB, NOTE_E4, 60);
  chime.endStep(400);

  chime.play();
}
```

## 19. 常见错误

### 忘记分号

错误：

```cpp
int speed = 100
```

正确：

```cpp
int speed = 100;
```

### `=` 和 `==` 混用

错误：

```cpp
if (speed = 0) {
}
```

正确：

```cpp
if (speed == 0) {
}
```

### 花括号没配对

错误：

```cpp
if (speed > 0) {
  m->forward(speed);
```

正确：

```cpp
if (speed > 0) {
  m->forward(speed);
}
```

### 字符和字符串混用

错误：

```cpp
if (motor == "a") {
}
```

正确：

```cpp
if (motor == 'a') {
}
```

因为 `motor` 是 `char`，单个字符要用单引号。

### 忘记调用 `chime.play()`

错误：

```cpp
chime.add(&motorA, NOTE_C4, 80, 300);
```

正确：

```cpp
chime.add(&motorA, NOTE_C4, 80, 300);
chime.play();
```

## 20. 编译时怎么看错误

运行：

```powershell
pio run
```

如果报错，重点看：

1. 报错文件名。
2. 报错行号。
3. 第一条真正的 `error:`。

例子：

```text
src/main.cpp:45:10: error: expected ';' before '}'
```

意思是：

| 部分 | 含义 |
|---|---|
| `src/main.cpp` | 错误在这个文件。 |
| `45` | 第 45 行附近。 |
| `expected ';'` | 缺少分号。 |

## 21. 你现在最需要掌握的最小集合

为了完成这个小船项目，你先掌握这些就够了：

1. 变量要写类型。
2. 语句结尾要写 `;`。
3. 代码块用 `{}`。
4. 判断用 `if / else if / else`。
5. 相等判断用 `==`。
6. 字符用单引号，例如 `'a'`。
7. 对象方法调用用 `.` 或 `->`。
8. Arduino 常用函数：`pinMode()`、`digitalWrite()`、`Serial.println()`。
9. 电机控制：`forward()`、`backward()`、`stop()`。
10. 旋律播放：`chime.add()`、`chime.play()`。

学会这些，你就可以读懂并完成 `src/main.cpp` 里的两个任务。

#ifndef BOAT_CONTROLLER_H
#define BOAT_CONTROLLER_H

// boat_controller.h 暴露给 main.cpp 的最小接口：
// setupBoatController() 负责初始化网络和服务器，loopBoatController() 负责循环处理连接。
// onMotorCommand()/onStartMotors() 则由 main.cpp 提供实现，控制器收到网页消息后会回调它们。

// ===========================================================================
//  BoatController  —  Edutech API for the InnoX Boat
//
//  Call these two functions from your main .ino / .cpp file:
//      setupBoatController();   // once in setup()
//      loopBoatController();    // every iteration of loop()
//
//  The system will loop-print the connection banner on Serial until a client
//  connects to the web page, so you never miss the SSID/IP.
//
//  ✏️  YOU MUST DEFINE these two functions:
//
//      onMotorCommand(char motor, int speed)
//          Called when the user drags a throttle slider.
//          motor : 'a' or 'b'
//          speed : -255 (full backward) … 0 (stop) … +255 (full forward)
//
//      onStartMotors()
//          Called once when the "START" button is pressed on the web page.
//          Play a chime sequence here, then set motorsEnabled = true.
// ===========================================================================

// 在 setup() 中调用一次，建立 Wi-Fi AP、网页服务器和 WebSocket 服务器。
void setupBoatController();
// 在 loop() 中反复调用，持续处理浏览器请求和 WebSocket 消息。
void loopBoatController();

// 返回是否曾经有浏览器连接过控制页面。
bool isClientConnected();

// main.cpp 必须定义下面两个函数；这里保留声明，让 boat_controller.cpp 能够调用学生代码。
void onMotorCommand(char motor, int speed);
void onStartMotors();

#endif // BOAT_CONTROLLER_H

#include "boat_controller.h"
#include "ap_manager.h"
#include "html.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// boat_controller.cpp 把“网页遥控器”和“学生写的电机函数”连接起来：
// 浏览器通过 WebSocket 发 JSON，解析后回调 main.cpp 中的 onStartMotors()/onMotorCommand()。

// ---------------------------------------------------------------------------
// Internal globals (hidden from the student)
// ---------------------------------------------------------------------------
// 管理 ESP32 SoftAP 的小工具类。
static APManager         apManager;
// 80 端口 HTTP 服务器，用来把 html.h 中的网页发给浏览器。
static WebServer         httpServer(80);
// 81 端口 WebSocket 服务器，用来实时收发油门和 START 指令。
static WebSocketsServer  wsServer(81);

// START 按下前不接受油门指令，避免网页刚连上就误转电机。
static bool              motorsEnabled = false;
// 记录是否曾经有客户端连上；没连上时串口会重复打印 SSID/IP。
static bool              clientWasEverConnected = false;
// 上一次打印连接提示 banner 的时间。
static unsigned long     lastBannerPrint = 0;

// ---------------------------------------------------------------------------
// WebSocket event → calls onMotorCommand() / onStartMotors()
// ---------------------------------------------------------------------------
static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    // 新浏览器连接后，记录状态并把当前 motorsEnabled 状态同步给它。
    clientWasEverConnected = true;
    Serial.println("[WS] Client connected");
    // Tell the client whether motors are already enabled
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"motorsEnabled\":%s}",
             motorsEnabled ? "true" : "false");
    wsServer.sendTXT(num, buf);
    return;
  }

  if (type == WStype_DISCONNECTED) {
    // 客户端断开时只记录日志；如果用于真实船，建议这里同时停止两个电机。
    Serial.println("[WS] Client disconnected");
    return;
  }

  if (type == WStype_TEXT) {
    // 所有浏览器命令都是 JSON 文本，例如 {"cmd":"start"} 或 {"motor":"a","speed":120}。
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (const char *)payload);

    if (err) {
      Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
      return;
    }

    // --- App-level keepalive ping → pong ------------------------------------
    if (doc["ping"]) {
      // 网页定期 ping，固件回复 pong，网页据此判断连接是否还活着。
      wsServer.sendTXT(num, "{\"pong\":1}");
      return;
    }

    // --- START command ------------------------------------------------------
    if (doc["cmd"] && strcmp(doc["cmd"], "start") == 0) {
      // START 只负责解锁电机并触发学生写的启动音效函数。
      motorsEnabled = true;
      onStartMotors();
      // Notify all clients
      wsServer.broadcastTXT("{\"motorsEnabled\":true}");
      return;
    }

    // --- Motor command (only if enabled) ------------------------------------
    const char *motorStr = doc["motor"];
    // 如果 JSON 里没有 speed 字段，ArduinoJson 的 | 0 会给默认值 0。
    int         speed    = doc["speed"] | 0;

    if (!motorStr || (motorStr[0] != 'a' && motorStr[0] != 'b')) {
      Serial.println("[WS] Invalid motor");
      return;
    }

    if (!motorsEnabled) {
      // START 前忽略油门，减少误操作风险。
      Serial.println("[WS] Ignored — motors not enabled");
      return;
    }

    // 把网页传来的速度限制到 PWM 可接受的 -255 到 +255。
    speed = constrain(speed, -255, 255);
    // 调用学生在 main.cpp 中实现的电机控制函数。
    onMotorCommand(motorStr[0], speed);
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void setupBoatController() {
  // --- WiFi AP -------------------------------------------------------------
  // 切到 AP 模式：ESP32 自己开热点，不需要外部路由器。
  apManager.setMode(WIFI_AP);
  // Default password — students can change this or add a setPassword() call
  // in main.cpp before setupBoatController() to override it.
  apManager.setPassword("innox1234");
  // 启动热点；APManager 会自动选择频道并打印 SSID/IP。
  apManager.begin();

  // --- HTTP server — serve the control page --------------------------------
  httpServer.on("/", []() {
    // 浏览器访问根路径时，返回嵌入在固件里的 INDEX_HTML。
    httpServer.send(200, "text/html", INDEX_HTML);
  });
  httpServer.begin();

  // --- WebSocket server ----------------------------------------------------
  wsServer.begin();
  wsServer.onEvent(onWsEvent);
  // Auto-ping every 3 s, wait 5 s for pong, disconnect after 3 missed pongs
  // WebSockets 库自带心跳；网页里还做了一层应用级 ping/pong。
  wsServer.enableHeartbeat(3000, 5000, 3);
}

bool isClientConnected() {
  // 这里返回“曾经连接过”，不是“当前仍然连接着”。
  return clientWasEverConnected;
}

void loopBoatController() {
  // HTTP 和 WebSocket 都需要在 loop() 里持续轮询处理。
  httpServer.handleClient();
  wsServer.loop();

  // Loop-print the banner every 3 seconds until a client connects
  if (!clientWasEverConnected) {
    unsigned long now = millis();
    if (now - lastBannerPrint >= 3000) {
      lastBannerPrint = now;
      Serial.printf("\n⛵ InnoX Boat ready at %s (SSID: %s)\n",
                    apManager.getIp().toString().c_str(),
                    apManager.getSsid());
      Serial.println("   Open the web page and tap START to enable motors.");
    }
  }
}

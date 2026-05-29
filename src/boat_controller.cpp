#include "boat_controller.h"
#include "ap_manager.h"
#include "html.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

// boat_controller.cpp 负责把网页遥控器和 main.cpp 里的电机控制函数连接起来。
// 浏览器通过 WebSocket 发送 JSON，固件解析后回调 onStartMotors()/onMotorCommand()。

// ---------------------------------------------------------------------------
// Internal globals (hidden from the student)
// ---------------------------------------------------------------------------
static APManager        apManager;
static WebServer        httpServer(80);
static WebSocketsServer wsServer(81);
static DNSServer        dnsServer;

static constexpr byte DNS_PORT = 53;

// START 按下前不接受油门指令，避免网页刚连上就误转电机。
static bool motorsEnabled = false;
// 记录是否曾经有客户端连上；没连上时串口会重复打印 SSID/IP。
static bool clientWasEverConnected = false;
// 上一次打印连接提示 banner 的时间。
static unsigned long lastBannerPrint = 0;

static String controlPageUrl() {
  return String("http://") + apManager.getIp().toString() + "/";
}

static void sendControlPage() {
  // no-store 避免手机 captive portal 小浏览器缓存旧页面。
  httpServer.sendHeader("Cache-Control", "no-store");
  httpServer.sendHeader("Connection", "close");
  httpServer.send(200, "text/html", INDEX_HTML);
}

static void redirectToControlPage() {
  // 手机系统访问检测地址时，把它带回 192.168.4.1 的控制页。
  httpServer.sendHeader("Location", controlPageUrl(), true);
  httpServer.sendHeader("Cache-Control", "no-store");
  httpServer.sendHeader("Connection", "close");
  httpServer.send(302, "text/plain", "Redirecting to InnoX Boat controller");
}

static void registerCaptivePortalRoutes() {
  httpServer.on("/", sendControlPage);

  // Android / ChromeOS connectivity checks.
  httpServer.on("/generate_204", redirectToControlPage);
  httpServer.on("/gen_204", redirectToControlPage);

  // iOS / macOS captive portal checks.
  httpServer.on("/hotspot-detect.html", redirectToControlPage);
  httpServer.on("/library/test/success.html", redirectToControlPage);

  // Windows connectivity checks.
  httpServer.on("/connecttest.txt", redirectToControlPage);
  httpServer.on("/ncsi.txt", redirectToControlPage);
  httpServer.on("/fwlink", redirectToControlPage);

  // Any unknown host/path should still land on the boat controller.
  httpServer.onNotFound(redirectToControlPage);
}

// ---------------------------------------------------------------------------
// WebSocket event -> calls onMotorCommand() / onStartMotors()
// ---------------------------------------------------------------------------
static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    // 新浏览器连接后，记录状态并同步当前 motorsEnabled 状态。
    clientWasEverConnected = true;
    Serial.println("[WS] Client connected");

    char buf[32];
    snprintf(buf, sizeof(buf), "{\"motorsEnabled\":%s}",
             motorsEnabled ? "true" : "false");
    wsServer.sendTXT(num, buf);
    return;
  }

  if (type == WStype_DISCONNECTED) {
    Serial.println("[WS] Client disconnected");
    return;
  }

  if (type == WStype_TEXT) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (const char *)payload);

    if (err) {
      Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
      return;
    }

    // App-level keepalive ping -> pong.
    if (doc["ping"]) {
      wsServer.sendTXT(num, "{\"pong\":1}");
      return;
    }

    // START command.
    if (doc["cmd"] && strcmp(doc["cmd"], "start") == 0) {
      motorsEnabled = true;
      onStartMotors();
      wsServer.broadcastTXT("{\"motorsEnabled\":true}");
      return;
    }

    // Motor command, only accepted after START.
    const char *motorStr = doc["motor"];
    int speed = doc["speed"] | 0;

    if (!motorStr || (motorStr[0] != 'a' && motorStr[0] != 'b')) {
      Serial.println("[WS] Invalid motor");
      return;
    }

    if (!motorsEnabled) {
      Serial.println("[WS] Ignored - motors not enabled");
      return;
    }

    speed = constrain(speed, -255, 255);
    onMotorCommand(motorStr[0], speed);
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void setupBoatController() {
  // ESP32 自己开热点，不需要外部路由器。
  apManager.setMode(WIFI_AP);
  apManager.setPassword("innox1234");
  apManager.begin();

  // Captive portal: make any domain resolve to the boat's AP IP.
  dnsServer.start(DNS_PORT, "*", apManager.getIp());
  Serial.printf("[DNS] Captive portal DNS started at %s\n",
                apManager.getIp().toString().c_str());

  registerCaptivePortalRoutes();
  httpServer.begin();

  wsServer.begin();
  wsServer.onEvent(onWsEvent);
  wsServer.enableHeartbeat(3000, 5000, 3);
}

bool isClientConnected() {
  // 这里返回“曾经连接过”，不是“当前仍然连接着”。
  return clientWasEverConnected;
}

void loopBoatController() {
  dnsServer.processNextRequest();
  httpServer.handleClient();
  wsServer.loop();

  // Loop-print the banner every 3 seconds until a client connects.
  if (!clientWasEverConnected) {
    unsigned long now = millis();
    if (now - lastBannerPrint >= 3000) {
      lastBannerPrint = now;
      Serial.printf("\nInnoX Boat ready at %s (SSID: %s)\n",
                    apManager.getIp().toString().c_str(),
                    apManager.getSsid());
      Serial.println("   Open the web page and tap START to enable motors.");
    }
  }
}

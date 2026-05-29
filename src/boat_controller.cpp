#include "boat_controller.h"
#include "ap_manager.h"
#include "boat_config.h"
#include "html.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <cstring>

// boat_controller.cpp owns networking only:
// Wi-Fi AP, captive portal HTTP, WebSocket state, and controller ownership.

// ---------------------------------------------------------------------------
// Internal globals
// ---------------------------------------------------------------------------
static APManager        apManager;
static WebServer        httpServer(80);
static WebSocketsServer wsServer(81);
static DNSServer        dnsServer;

static constexpr byte DNS_PORT = 53;
static constexpr int  NO_CONTROLLER = -1;

static bool motorsEnabled = false;
static int activeController = NO_CONTROLLER;
static unsigned long lastDrivePacketAt = 0;

static bool clientWasEverConnected = false;
static unsigned long lastBannerPrint = 0;
static unsigned long lastStaReconnectAttempt = 0;

static String controlPageUrl() {
  return String("http://") + apManager.getIp().toString() + "/";
}

static bool isStationConfigured() {
  return BOAT_STA_ENABLED && std::strlen(BOAT_STA_SSID) > 0;
}

static bool isStationConnected() {
  return isStationConfigured() && WiFi.status() == WL_CONNECTED;
}

static void connectStationNetworkBlocking() {
  if (!isStationConfigured()) return;

  Serial.printf("[STA] Connecting to hotspot \"%s\"...\n", BOAT_STA_SSID);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(BOAT_STA_SSID, BOAT_STA_PASSWORD);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < BOAT_STA_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[STA] Connected. IP: %s  Channel: %d  RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.channel(),
                  WiFi.RSSI());
  } else {
    Serial.println("[STA] Hotspot not connected yet; boat AP remains available.");
    WiFi.disconnect(false);
  }
}

static void configureAccessPointChannel() {
  if (isStationConnected()) {
    // ESP32 has one 2.4 GHz radio, so AP+STA must share one channel.
    apManager.setChannel((uint8_t)WiFi.channel());
    Serial.printf("[AP] Using hotspot channel %d for AP+STA stability\n",
                  WiFi.channel());
    return;
  }

  if (BOAT_WIFI_CHANNEL != 0) {
    apManager.setChannel(BOAT_WIFI_CHANNEL);
  }
}

static void maintainStationConnection() {
  if (!isStationConfigured() || WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastStaReconnectAttempt < BOAT_STA_RECONNECT_INTERVAL_MS) return;

  lastStaReconnectAttempt = now;
  Serial.printf("[STA] Reconnecting to hotspot \"%s\"...\n", BOAT_STA_SSID);
  WiFi.begin(BOAT_STA_SSID, BOAT_STA_PASSWORD);
}

static void printReadyBanner() {
  Serial.println();
  Serial.println("InnoX Boat ready");
  Serial.printf("   Boat AP: http://%s/ (SSID: %s, password: innox1234)\n",
                apManager.getIp().toString().c_str(),
                apManager.getSsid());

  if (isStationConfigured()) {
    if (isStationConnected()) {
      Serial.printf("   Hotspot LAN: http://%s/ (connected to %s)\n",
                    WiFi.localIP().toString().c_str(),
                    BOAT_STA_SSID);
    } else {
      Serial.printf("   Hotspot LAN: not connected to %s; use Boat AP.\n",
                    BOAT_STA_SSID);
    }
  }

  Serial.println("   Open the web page and tap START to enable motors.");
}

static void sendControlPage() {
  httpServer.sendHeader("Cache-Control", "no-store");
  httpServer.sendHeader("Connection", "close");
  httpServer.send(200, "text/html", INDEX_HTML);
}

static void redirectToControlPage() {
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

  httpServer.onNotFound(redirectToControlPage);
}

static void sendState(uint8_t num) {
  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"clientId\":%u,\"motorsEnabled\":%s,\"activeController\":%d}",
           num,
           motorsEnabled ? "true" : "false",
           activeController);
  wsServer.sendTXT(num, buf);
}

static void broadcastState() {
  char buf[80];
  snprintf(buf, sizeof(buf),
           "{\"motorsEnabled\":%s,\"activeController\":%d}",
           motorsEnabled ? "true" : "false",
           activeController);
  wsServer.broadcastTXT(buf);
}

static bool isActiveController(uint8_t num) {
  return activeController == (int)num;
}

static void stopForSafety(const char *reason) {
  if (!motorsEnabled && activeController == NO_CONTROLLER) return;

  Serial.printf("[SAFE] %s - stopping motors\n", reason);
  onControllerLost();
  motorsEnabled = false;
  activeController = NO_CONTROLLER;
  lastDrivePacketAt = 0;
  broadcastState();
}

static void handleStartCommand(uint8_t num) {
  if (activeController != NO_CONTROLLER && !isActiveController(num)) {
    Serial.printf("[WS] START ignored - controller %d already active\n", activeController);
    sendState(num);
    return;
  }

  activeController = num;
  motorsEnabled = true;
  lastDrivePacketAt = millis();

  Serial.printf("[WS] Client %u became active controller\n", num);
  onStartMotors();
  broadcastState();
}

static void handleDriveCommand(uint8_t num, JsonDocument &doc) {
  if (!motorsEnabled || !isActiveController(num)) {
    Serial.printf("[WS] Drive ignored from client %u\n", num);
    sendState(num);
    return;
  }

  int speedA = constrain(doc["a"] | 0, -255, 255);
  int speedB = constrain(doc["b"] | 0, -255, 255);

  lastDrivePacketAt = millis();
  onDriveCommand(speedA, speedB);
}

static void handleLegacyMotorCommand(uint8_t num, JsonDocument &doc) {
  const char *motorStr = doc["motor"];
  int speed = doc["speed"] | 0;

  if (!motorStr || (motorStr[0] != 'a' && motorStr[0] != 'b')) {
    Serial.println("[WS] Invalid motor");
    return;
  }

  if (!motorsEnabled || !isActiveController(num)) {
    Serial.printf("[WS] Legacy motor ignored from client %u\n", num);
    sendState(num);
    return;
  }

  lastDrivePacketAt = millis();
  onMotorCommand(motorStr[0], constrain(speed, -255, 255));
}

// ---------------------------------------------------------------------------
// WebSocket event -> calls main.cpp callbacks
// ---------------------------------------------------------------------------
static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    clientWasEverConnected = true;
    Serial.printf("[WS] Client %u connected\n", num);
    sendState(num);
    return;
  }

  if (type == WStype_DISCONNECTED) {
    Serial.printf("[WS] Client %u disconnected\n", num);
    if (isActiveController(num)) {
      stopForSafety("active controller disconnected");
    }
    return;
  }

  if (type != WStype_TEXT) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, (const char *)payload);

  if (err) {
    Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
    return;
  }

  if (doc["ping"]) {
    wsServer.sendTXT(num, "{\"pong\":1}");
    return;
  }

  const char *cmd = doc["cmd"];

  if (cmd && strcmp(cmd, "start") == 0) {
    handleStartCommand(num);
    return;
  }

  if (cmd && strcmp(cmd, "drive") == 0) {
    handleDriveCommand(num, doc);
    return;
  }

  if (doc["motor"]) {
    handleLegacyMotorCommand(num, doc);
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void setupBoatController() {
  if (isStationConfigured()) {
    connectStationNetworkBlocking();
    apManager.setMode(WIFI_AP_STA);
  } else {
    apManager.setMode(WIFI_AP);
  }

  apManager.setPassword("innox1234");
  configureAccessPointChannel();
  apManager.begin();

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
  return clientWasEverConnected;
}

void loopBoatController() {
  dnsServer.processNextRequest();
  httpServer.handleClient();
  wsServer.loop();
  maintainStationConnection();

  if (motorsEnabled && activeController != NO_CONTROLLER) {
    unsigned long now = millis();
    if (now - lastDrivePacketAt > CONTROL_TIMEOUT_MS) {
      stopForSafety("drive packet timeout");
    }
  }

  if (!clientWasEverConnected) {
    unsigned long now = millis();
    if (now - lastBannerPrint >= 3000) {
      lastBannerPrint = now;
      printReadyBanner();
    }
  }
}

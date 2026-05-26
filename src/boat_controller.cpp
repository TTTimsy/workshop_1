#include "boat_controller.h"
#include "ap_manager.h"
#include "html.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Internal globals (hidden from the student)
// ---------------------------------------------------------------------------
static APManager         apManager;
static WebServer         httpServer(80);
static WebSocketsServer  wsServer(81);

static bool              motorsEnabled = false;
static bool              clientWasEverConnected = false;
static unsigned long     lastBannerPrint = 0;

// ---------------------------------------------------------------------------
// WebSocket event → calls onMotorCommand() / onStartMotors()
// ---------------------------------------------------------------------------
static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
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

    // --- START command ------------------------------------------------------
    if (doc["cmd"] && strcmp(doc["cmd"], "start") == 0) {
      motorsEnabled = true;
      onStartMotors();
      // Notify all clients
      wsServer.broadcastTXT("{\"motorsEnabled\":true}");
      return;
    }

    // --- Motor command (only if enabled) ------------------------------------
    const char *motorStr = doc["motor"];
    int         speed    = doc["speed"] | 0;

    if (!motorStr || (motorStr[0] != 'a' && motorStr[0] != 'b')) {
      Serial.println("[WS] Invalid motor");
      return;
    }

    if (!motorsEnabled) {
      Serial.println("[WS] Ignored — motors not enabled");
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
  // --- WiFi AP -------------------------------------------------------------
  apManager.setMode(WIFI_AP);
  apManager.begin();

  // --- HTTP server — serve the control page --------------------------------
  httpServer.on("/", []() {
    httpServer.send(200, "text/html", INDEX_HTML);
  });
  httpServer.begin();

  // --- WebSocket server ----------------------------------------------------
  wsServer.begin();
  wsServer.onEvent(onWsEvent);
}

bool isClientConnected() {
  return clientWasEverConnected;
}

void loopBoatController() {
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

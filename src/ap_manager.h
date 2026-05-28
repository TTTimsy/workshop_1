#ifndef AP_MANAGER_H
#define AP_MANAGER_H

// Arduino.h 提供 Arduino 框架的基础类型和函数，例如 String、delay()、Serial、IPAddress 等。
#include <Arduino.h>
// WiFi.h 提供 ESP32 的 Wi-Fi API，例如 WiFi.mode()、WiFi.softAP()、WiFi.scanNetworks() 等。
#include <WiFi.h>


// ---------------------------------------------------------------------------
// DO NOT TOUCH THIS FILE! — it's a simple wrapper around WiFi.softAP() to manage our AP

// THIS IS OUT OF SCOPE FOR STUDENTS
// ---------------------------------------------------------------------------


// APManager 负责把 ESP32 配置成一个 Wi-Fi 热点，供手机/电脑连接到小船控制页面。
class APManager {
public:
  // 构造函数会设置默认频道、默认 Wi-Fi 模式，并根据芯片 MAC 地址生成唯一 SSID。
  APManager();

  // Configuration
  // 设置热点名称，例如 "InnoX-Boat-1234"。
  void setSsid(const char *ssid);
  // 设置热点密码；传入空字符串时会启动开放热点。
  void setPassword(const char *password);
  // 设置热点频道；0 表示 begin() 时自动扫描并选择较空闲的频道。
  void setChannel(uint8_t channel);   // 0 = auto-select on begin()
  // 设置 Wi-Fi 工作模式，项目里主要使用 WIFI_AP。
  void setMode(wifi_mode_t mode);     // e.g. WIFI_AP, WIFI_AP_STA

  // Actions
  // 扫描周围 2.4 GHz Wi-Fi，返回网络数量最少的频道。
  int  findBestChannel();             // scan & return least-congested 2.4 GHz ch
  // 按当前 SSID、密码、频道和模式启动热点。
  bool begin();                       // auto-select channel if 0, then start AP

  // Getters
  // 返回当前热点名称。
  const char *getSsid() const;
  // 返回当前热点频道。
  uint8_t     getChannel() const;
  // 返回 ESP32 热点的 IP，通常是 192.168.4.1。
  IPAddress   getIp() const;

  // Static helpers
  // 根据芯片 MAC 地址生成形如 "InnoX-Boat-XXXX" 的唯一热点名称。
  static void generateSsidFromMac(char *buf, size_t len);

private:
  // 保存热点名称，最多 31 个字符再加字符串结束符。
  char        _ssid[32];
  // 保存热点密码，最多 63 个字符再加字符串结束符。
  char        _password[64];
  // 保存热点频道；0 代表尚未指定，需要自动选择。
  uint8_t     _channel;               // 0 = auto-select on begin()
  // 保存 Wi-Fi 模式，例如 WIFI_AP 或 WIFI_AP_STA。
  wifi_mode_t _wifiMode;
};

#endif // AP_MANAGER_H

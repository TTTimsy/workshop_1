#include "ap_manager.h"

// 这个文件实现 APManager：扫描频道、启动 ESP32 热点、生成唯一 SSID。
// 它属于项目底层网络代码，学生通常只需要知道最终会得到一个可连接的 Wi-Fi 热点。

// ---------------------------------------------------------------------------
// DO NOT TOUCH THIS FILE! — it's a simple wrapper around WiFi.softAP() to manage our AP

// THIS IS OUT OF SCOPE FOR STUDENTS
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Constructor — generates an SSID from the MAC and defaults to open AP on ch 0
// ---------------------------------------------------------------------------
APManager::APManager()
  : _channel(0)
  , _wifiMode(WIFI_AP)
{
  // 空密码表示默认开放热点；setupBoatController() 会把它改成 innox1234。
  _password[0] = '\0';
  // 用芯片唯一 MAC 地址生成默认 SSID，避免多艘船重名。
  generateSsidFromMac(_ssid, sizeof(_ssid));
}

// ---------------------------------------------------------------------------
// Configuration setters
// ---------------------------------------------------------------------------
void APManager::setSsid(const char *ssid) {
  // strncpy 限制复制长度，防止外部传入过长 SSID 写爆数组。
  strncpy(_ssid, ssid, sizeof(_ssid) - 1);
  // 手动补字符串结束符，保证 _ssid 一定是合法 C 字符串。
  _ssid[sizeof(_ssid) - 1] = '\0';
}

void APManager::setPassword(const char *password) {
  // WPA2 密码最长 63 字符，这里预留 1 个字节给字符串结束符。
  strncpy(_password, password, sizeof(_password) - 1);
  // 确保密码字符串结尾安全。
  _password[sizeof(_password) - 1] = '\0';
}

void APManager::setChannel(uint8_t channel) {
  // 保存用户指定频道；如果是 0，begin() 会自动选择频道。
  _channel = channel;
}

void APManager::setMode(wifi_mode_t mode) {
  // 保存 Wi-Fi 模式，后续 begin() 会传给 WiFi.mode()。
  _wifiMode = mode;
}

// ---------------------------------------------------------------------------
// findBestChannel — scan 2.4 GHz channels 1-13, return the least congested
// ---------------------------------------------------------------------------
int APManager::findBestChannel() {
  // ESP32 2.4 GHz 常用频道范围是 1 到 13。
  static constexpr int NUM_CHANNELS = 13;
  // 每个数组元素记录对应频道扫描到多少个热点。
  int channelCounts[NUM_CHANNELS] = {0};

  Serial.println("[AP] Scanning WiFi channels for least congestion...");

  // Perform a full scan — 200ms per channel is plenty
  // false 表示同步扫描，true 表示显示隐藏网络，200 是每个频道扫描毫秒数。
  int n = WiFi.scanNetworks(false, true, false, 200);

  if (n == WIFI_SCAN_FAILED) {
    // 扫描失败时选择常见中间频道 6，保证热点仍能启动。
    Serial.println("[AP] WiFi scan failed — falling back to channel 6");
    WiFi.scanDelete();
    return 6;
  }

  // Tally networks per channel
  for (int i = 0; i < n; i++) {
    // 读取第 i 个扫描结果所在频道。
    int8_t ch = WiFi.channel(i);
    if (ch >= 1 && ch <= NUM_CHANNELS) {
      // 把 1-13 频道转换成 0-12 数组下标并计数。
      channelCounts[ch - 1]++;
    }
  }

  // Pick the channel with the fewest networks (ties → lowest channel)
  int bestChannel = 1;
  int minCount = channelCounts[0];
  for (int i = 1; i < NUM_CHANNELS; i++) {
    if (channelCounts[i] < minCount) {
      // 找到更少网络的频道就更新当前最佳选择。
      minCount = channelCounts[i];
      bestChannel = i + 1;
    }
  }

  // 释放扫描结果占用的内存。
  WiFi.scanDelete();

#ifdef DEBUG_AP_SCAN
  Serial.println("[AP] Channel scan results:");
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.printf("  ch %2d: %d networks\n", i + 1, channelCounts[i]);
  }
  Serial.printf("[AP] Best channel: %d (least congested)\n", bestChannel);
#endif

  return bestChannel;
}

// ---------------------------------------------------------------------------
// begin — auto-select channel if 0, then start the AP
// ---------------------------------------------------------------------------
bool APManager::begin() {
  // MUST set WiFi mode before scanning — otherwise scan hangs on ESP32
  // 先设置 Wi-Fi 模式，否则 ESP32 上直接扫描可能卡住。
  WiFi.mode(_wifiMode);
  delay(100);

  // Auto-select channel if not explicitly set
  if (_channel == 0) {
    // 没有手动指定频道时，自动找一个周围网络较少的频道。
    int ch = findBestChannel();
    _channel = (uint8_t)ch;
  }

  bool ok;
  if (_password[0] == '\0') {
    // Open AP (no password)
    // nullptr 密码表示开放热点。
    ok = WiFi.softAP(_ssid, nullptr, _channel);
  } else {
    // 带密码启动 WPA/WPA2 热点。
    ok = WiFi.softAP(_ssid, _password, _channel);
  }

  Serial.printf("[AP] SSID: \"%s\"  Channel: %u  IP: %s  %s\n",
                _ssid, _channel,
                WiFi.softAPIP().toString().c_str(),
                ok ? "OK" : "FAILED");

  return ok;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
const char *APManager::getSsid() const {
  // 返回内部 SSID 字符数组的只读指针。
  return _ssid;
}

uint8_t APManager::getChannel() const {
  // 返回当前频道号。
  return _channel;
}

IPAddress APManager::getIp() const {
  // 返回 SoftAP 默认 IP；浏览器访问这个 IP 能打开控制页面。
  return WiFi.softAPIP();
}

// ---------------------------------------------------------------------------
// Static: generate an SSID like "InnoX-Boat-XXXX" from the MAC address
// ---------------------------------------------------------------------------
void APManager::generateSsidFromMac(char *buf, size_t len) {
  // 读取芯片 eFuse 中的唯一 MAC 值。
  uint64_t mac = ESP.getEfuseMac();
  // 取 MAC 的一部分作为短 ID，方便显示在 SSID 末尾。
  uint16_t chipId = (uint16_t)(mac >> 24);
  // 写入目标缓冲区，snprintf 会按 len 限制长度。
  snprintf(buf, len, "InnoX-Boat-%04X", chipId);
}

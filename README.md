# ESP32 AI 聊天机器人 by Darren.X

基于ESP32实现的AI聊天机器人项目，通过Coze API实现智能对话交互。

## 功能特性

- ✅ Wi-Fi自动连接与重试机制
- 🔄 Coze API多阶段交互（创建会话/轮询状态/获取结果）
- 📡 HTTPS安全通信（SSL证书验证）
- 📊 实时内存使用监控
- 📦 JSON数据动态解析（使用cJSON库）
- 🔄 异步任务处理机制

## 硬件要求

- ESP32开发板（支持Wi-Fi）
- 电源供应（USB/电池）
- 可选：音频输出设备（用于MP3播放）

## 环境配置

1. **开发环境**
   - ESP-IDF v4.4+
   - 支持SPIRAM的编译配置（可选）

2. **依赖库**
   - [cJSON](https://github.com/DaveGamble/cJSON) (已包含在组件目录)
   - FreeRTOS
   - esp_http_client
   - esp_wifi

3. **配置文件**
   修改以下宏定义：
   ```c
   #define APP_ESP_WIFI_SSID "YOUR_WIFI_SSID"
   #define APP_ESP_WIFI_PASS "YOUR_WIFI_PASSWORD"
   #define APP_HTTP_TOKEN "YOUR_API_TOKEN"
   #define s_chat.bot_id "YOUR_BOT_ID"

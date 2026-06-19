# esp32-smart-trap-v5.4
4G 模組誘捕籠系統 - 結合 Google Sheet 日誌與 Email 警報功能

## 專案雲端後台設定
* **Google GAS API 網址**: `(https://script.google.com/macros/s/AKfycbwXilOYc8-pPnzFD1gosnh6lbjmpjn_t0-9vMeRUpy9AGFp42Ck3YcWB45xLURxl_Ln/exec)`
* **功能**: 接收 ESP32 POST 請求，寫入試算表，偵測到 TRIGGERED 自動發送 Email。


Trap Webhook
https://hook.eu1.make.com/txe5b61dmn7alzogp9xmup7apicl8k6p

# 🚨 智慧物聯網誘捕籠系統 (Smart Trap System)

本專案利用 MQTT 協定與 Google 雲端服務，打造 24 小時自動化監控與 Email 即時報警系統。

## 🌐 系統架構
1. **硬體端/測試端**: 使用 MQTT 協定發射狀態至 `broker.emqx.io`。
2. **中繼端 (Google Cloud Run)**: Python 監聽特定主題，解析封包並轉換為中文通知。
3. **後端 (Google Apps Script)**: 接收雲端請求，自動寫入試算表並發送 Gmail 警報。

## 📁 檔案說明
* `cloud-run/`: 包含 Python 看門狗程式與依賴套件。
* `google-apps-script/`: 包含試算表寫入與 MailApp 發信邏輯。
* `hardware/`: (預留) 未來硬體端燒錄代碼。

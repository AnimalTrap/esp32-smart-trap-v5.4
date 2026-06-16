# esp32-smart-trap-v5.4
4G 模組誘捕籠系統 - 結合 Google Sheet 日誌與 Email 警報功能

## 專案雲端後台設定
* **Google GAS API 網址**: `(https://script.google.com/macros/s/AKfycbwXilOYc8-pPnzFD1gosnh6lbjmpjn_t0-9vMeRUpy9AGFp42Ck3YcWB45xLURxl_Ln/exec)`
* **功能**: 接收 ESP32 POST 請求，寫入試算表，偵測到 TRIGGERED 自動發送 Email。


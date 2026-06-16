#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RXD2 16
#define TXD2 17
#define MAGNET_SENSOR_PIN 13  

bool currentStatusIsArmed = true; 
String csqSignal = "N/A";
String cellInfo = "NONE"; 
bool isMqttConnected = false; 

void updateOLED(String trapStatus) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("== ALIVE TRAP v5.4 ==");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 18);
  display.print("4G: "); display.print(csqSignal);
  display.print(isMqttConnected ? " [ONLINE]" : " [OFFLINE]");
  display.drawLine(0, 32, 128, 32, SSD1306_WHITE);
  display.setCursor(0, 42);
  display.setTextSize(2);
  if (trapStatus == "ARMED") display.println("  ARMED");
  else display.println("!TRIGGER!");
  display.display();
}

String sendAT_and_GetReply(String cmd, int delayTime) {
  Serial2.println(cmd);
  delay(delayTime);
  String response = "";
  while (Serial2.available()) {
    char c = Serial2.read();
    response += c;
  }
  Serial.print(response); 
  return response;
}

void connectToMqtt() {
  Serial.println("\n🌐 正在嘗試連線至 MQTT Broker...");
  isMqttConnected = false;
  
  sendAT_and_GetReply("AT+QMTCLOSE=4", 500);
  sendAT_and_GetReply("AT+QMTCFG=\"ssl\",4,0", 500); 
  sendAT_and_GetReply("AT+QMTCFG=\"version\",4,4", 500);
  
  String openRes = sendAT_and_GetReply("AT+QMTOPEN=4,\"broker.emqx.io\",1883", 4000);
  if (openRes.indexOf("+QMTOPEN: 4,0") >= 0 || openRes.indexOf("+QMTOPEN: 4,2") >= 0) {
    String randomClientId = "esp32_trap_" + String(random(1000, 9999));
    String connCmd = "AT+QMTCONN=4,\"" + randomClientId + "\",\"\",\"\",1";
    
    String connRes = sendAT_and_GetReply(connCmd, 4000);
    if (connRes.indexOf("+QMTCONN: 4,0,0") >= 0) {
      Serial.println("🏆 MQTT 連線成功！");
      sendAT_and_GetReply("AT+QMTSUB=4,1,\"hk_trap_2026_xyz9527/status\",0", 1500);
      isMqttConnected = true;
    }
  }
}

// ⚠️ 關鍵修復：移除引號確保 MQTT 發送不報錯
void checkCellStation() {
  String res = sendAT_and_GetReply("AT+QENG=\"servingcell\"", 1000);
  int startIdx = res.indexOf("+QENG:");
  if (startIdx >= 0) {
    int endIdx = res.indexOf("\r", startIdx);
    if (endIdx > startIdx) {
      String pureData = res.substring(startIdx, endIdx);
      pureData.replace("\"", ""); // 強制移除所有引號
      cellInfo = pureData;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  pinMode(MAGNET_SENSOR_PIN, INPUT_PULLUP); 
  randomSeed(analogRead(0)); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.println("Booting 4G v5.4...");
  display.display();
  
  delay(3000);

  sendAT_and_GetReply("AT+QIDEACT=1", 500); 
  sendAT_and_GetReply("AT+QICSGP=1,1,\"share.hk.link\",\"\",\"\",0", 2000);
  sendAT_and_GetReply("AT+QIACT=1", 3000);
  
  delay(1000);
  sendAT_and_GetReply("AT+CSQ", 500);

  checkCellStation();
  connectToMqtt();

  if (isMqttConnected && cellInfo != "NONE") {
    Serial2.println("AT+QMTPUB=4,0,0,0,\"hk_trap_2026_xyz9527/status\",\"" + cellInfo + "\"");
    delay(1000);
  }

  if (digitalRead(MAGNET_SENSOR_PIN) == LOW) { currentStatusIsArmed = true; updateOLED("ARMED"); } 
  else { currentStatusIsArmed = false; updateOLED("TRIGGERED"); }
}

void loop() {
  while (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    Serial.println(incoming); 
    if (incoming.indexOf("+QMTSTAT:") >= 0) {
      Serial.println("\n⚠️ 偵測到 4G 模組 MQTT 鏈接斷開！觸發自動重連...");
      isMqttConnected = false;
    }
  }

  static unsigned long lastRetryCheck = 0;
  if (!isMqttConnected && (millis() - lastRetryCheck > 10000)) {
    connectToMqtt();
    if (isMqttConnected && cellInfo != "NONE") {
      Serial2.println("AT+QMTPUB=4,0,0,0,\"hk_trap_2026_xyz9527/status\",\"" + cellInfo + "\"");
    }
    lastRetryCheck = millis();
  }

  static unsigned long lastSignalCheck = 0;
  if (millis() - lastSignalCheck > 30000) {
    sendAT_and_GetReply("AT+CSQ", 500);
    updateOLED(currentStatusIsArmed ? "ARMED" : "TRIGGERED");
    lastSignalCheck = millis();
  }

  int sensorValue = digitalRead(MAGNET_SENSOR_PIN);
  if (sensorValue == HIGH && currentStatusIsArmed) {
    delay(50);
    if (digitalRead(MAGNET_SENSOR_PIN) == HIGH) { 
      currentStatusIsArmed = false;
      updateOLED("TRIGGERED");
      if(isMqttConnected) {
        Serial2.println("AT+QMTPUB=4,0,0,0,\"hk_trap_2026_xyz9527/status\",\"WARNING: TRAP_TRIGGERED\"");
      }
    }
  } 
  else if (sensorValue == LOW && !currentStatusIsArmed) {
    delay(50);
    if (digitalRead(MAGNET_SENSOR_PIN) == LOW) {
      currentStatusIsArmed = true;
      updateOLED("ARMED");
      if(isMqttConnected) {
        Serial2.println("AT+QMTPUB=4,0,0,0,\"hk_trap_2026_xyz9527/status\",\"STATUS: TRAP_ARMED\"");
      }
    }
  }

  if (Serial.available()) { Serial2.write(Serial.read()); }
}

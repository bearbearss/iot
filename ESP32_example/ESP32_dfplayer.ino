// ============================================================
// Smart Bath Guard — ESP32 메인 펌웨어
// ============================================================
// ★ 설정 변경은 config.h 파일만 수정하세요 ★
//
// 라이브러리 설치 (Arduino IDE → 라이브러리 관리):
//   - DFRobotDFPlayerMini
//   - ArduinoJson
//
// 부품 연결:
//   DFPlayer TX  → GPIO16 (RX2)          [1KΩ 불필요]
//   DFPlayer RX  → GPIO17 (TX2)          [1KΩ 직렬 필수]
//   DFPlayer VCC → 5V,  GND → GND
//   BL0303 문센서 → GPIO27 (INPUT_PULLUP)
//   아케이드 버튼 → GPIO34 (INPUT_PULLUP)
//   내장 LED     → GPIO2  (WiFi 상태 표시)
// ============================================================

#include "config.h"            // ★ 설정 파일

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobotDFPlayerMini.h>

// ── DFPlayer ──────────────────────────────────────────────
HardwareSerial dfSerial(2);   // UART2: RX=16, TX=17
DFRobotDFPlayerMini myDF;

int  currentTrack  = 0;
bool isPlaying     = false;
bool fallSequence  = false;

// ── 타이머 ────────────────────────────────────────────────
unsigned long lastDataMs   = 0;
unsigned long lastAudioMs  = 0;
unsigned long lastButtonMs = 0;

// ── 버튼 상태 ─────────────────────────────────────────────
bool buttonPrev = HIGH;   // INPUT_PULLUP: HIGH = 안 눌림

// ── WiFi 재연결 ───────────────────────────────────────────
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  DBGLN("\n[WiFi] 연결 끊김 — 재연결 시도...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < WIFI_RETRY_MAX) {
    delay(500);
    DBG(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    DBGF("\n[WiFi] 재연결 성공: %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(LED_PIN, HIGH);
  } else {
    DBGLN("\n[WiFi] 재연결 실패 — 5초 후 재시도");
    digitalWrite(LED_PIN, LOW);
    delay(5000);
  }
}


// ════════════════════════════════════════════════════════
// setup()
// ════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(300);
  DBGLN("\n\n========================================");
  DBGLN("  Smart Bath Guard — ESP32 부팅");
  DBGLN("========================================");

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 문센서
  pinMode(DOOR_PIN, INPUT_PULLUP);
  DBGLN("[OK] 문센서(BL0303) GPIO" + String(DOOR_PIN));

  // 아케이드 버튼
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  DBGLN("[OK] 아케이드 버튼 GPIO" + String(BUTTON_PIN));

  // DFPlayer
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  if (!myDF.begin(dfSerial)) {
    DBGLN("[ERROR] DFPlayer 초기화 실패");
    DBGLN("  → SD카드 삽입 여부, 1KΩ 저항 확인");
  } else {
    DBGLN("[OK] DFPlayer 초기화 완료");
    myDF.volume(15);
    myDF.EQ(DFPLAYER_EQ_NORMAL);
  }

  // WiFi
  DBGF("[WiFi] SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); DBG(".");
    if (++retry > WIFI_RETRY_MAX * 2) {
      DBGLN("\n[ERROR] WiFi 연결 실패");
      DBGLN("  → WIFI_SSID / WIFI_PASSWORD 확인 (config.h)");
      DBGLN("  → 10초 후 재부팅...");
      delay(10000);
      ESP.restart();
    }
  }

  DBGF("\n[WiFi] 연결됨: %s\n", WiFi.localIP().toString().c_str());
  DBGF("[Server] http://%s:%d\n", SERVER_IP, SERVER_PORT);
  DBGLN("[Server] 설정 확인: GET /esp32/config");
  DBGLN("========================================\n");

  digitalWrite(LED_PIN, HIGH);   // 연결 성공 → LED ON
}


// ════════════════════════════════════════════════════════
// loop()
// ════════════════════════════════════════════════════════
void loop() {
  ensureWiFi();   // WiFi 끊기면 자동 재연결

  unsigned long now = millis();

  // 1. 오디오 명령 폴링 (500ms)
  if (now - lastAudioMs >= AUDIO_INTERVAL_MS) {
    lastAudioMs = now;
    pollAudioCommand();
  }

  // 2. 센서 데이터 전송 (3000ms)
  if (now - lastDataMs >= DATA_INTERVAL_MS) {
    lastDataMs = now;
    sendSensorData();
  }

  // 3. 아케이드 버튼 감지 (디바운스 50ms)
  if (now - lastButtonMs >= 50) {
    lastButtonMs = now;
    checkButton();
  }

  // 4. DFPlayer 트랙 완료 감지
  checkPlayFinished();

  delay(10);
}


// ════════════════════════════════════════════════════════
// 오디오 명령 폴링
// ════════════════════════════════════════════════════════
void pollAudioCommand() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/esp32/audio-command";
  http.begin(url);
  http.setTimeout(800);

  int code = http.GET();
  if (code == 200) {
    parseAndExecuteCommand(http.getString());
  } else if (code != -1) {
    DBGF("[AUDIO] 폴링 실패: HTTP %d\n", code);
  }
  http.end();
}

void parseAndExecuteCommand(const String& json) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return;
  const char* cmd = doc["command"];
  if (!cmd || strcmp(cmd, "none") == 0) return;

  if (strcmp(cmd, "play") == 0) {
    playTrack(doc["track"] | 1);
  } else if (strcmp(cmd, "stop") == 0) {
    myDF.stop();
    isPlaying = false; currentTrack = 0; fallSequence = false;
    DBGLN("[AUDIO] 정지");
  } else if (strcmp(cmd, "volume") == 0) {
    int vol = constrain((int)(doc["value"] | 15), 0, 30);
    myDF.volume(vol);
    DBGF("[AUDIO] 볼륨: %d\n", vol);
  }
}

void playTrack(int track) {
  myDF.playMp3Folder(track);
  isPlaying = true; currentTrack = track;
  if (track == 2) fallSequence = true;
  DBGF("[AUDIO] 재생: /MP3/000%d.mp3\n", track);
}


// ════════════════════════════════════════════════════════
// 낙상 순차 재생 (트랙 2→3→5)
// ════════════════════════════════════════════════════════
void checkPlayFinished() {
  if (!fallSequence || !isPlaying || !myDF.available()) return;
  int type  = myDF.readType();
  int value = myDF.read();
  if (type != DFPlayerPlayFinished) return;

  DBGF("[AUDIO] 트랙 %d 완료\n", value);
  if      (value == 2) { delay(500);  playTrack(3); }
  else if (value == 3) { delay(3000); playTrack(5); }
  else if (value == 5) { fallSequence = false; isPlaying = false;
                         DBGLN("[AUDIO] 낙상 안내 시퀀스 완료"); }
}


// ════════════════════════════════════════════════════════
// 아케이드 버튼 감지
// ════════════════════════════════════════════════════════
void checkButton() {
  bool btn = digitalRead(BUTTON_PIN);
  if (buttonPrev == HIGH && btn == LOW) {   // 눌림 감지 (INPUT_PULLUP)
    DBGLN("[BUTTON] 아케이드 버튼 눌림 → POST /button/press");
    postButtonPress();
  }
  buttonPrev = btn;
}

void postButtonPress() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/button/press";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST("{}");
  DBGF("[BUTTON] 서버 응답: HTTP %d\n", code);
  http.end();
}


// ════════════════════════════════════════════════════════
// 센서 데이터 전송
// ════════════════════════════════════════════════════════
void sendSensorData() {
  if (WiFi.status() != WL_CONNECTED) return;

  // ── 실제 센서 읽기 코드를 여기에 삽입 ────────────────────
  // float height = tof_sensor.readRange();  // VL53L0X ToF 센서
  // int   angle  = pico_uart.getAngle();    // Pico → UART
  // bool  mmwave = mmwave_sensor.detected();
  // bool  fallC  = (height < 50.0 && mmwave);
  // bool  fallD  = (height < 20.0 && mmwave);
  // ──────────────────────────────────────────────────────────
  // 지금은 더미값 (실제 센서 코드로 교체하세요):
  float height = 165.0;
  int   angle  = 88;
  bool  mmwave = true;
  bool  fallCandidate = false;
  bool  fallDetected  = false;

  // 문센서 (BL0303 자석 스위치)
  bool   doorOpen  = digitalRead(DOOR_PIN) == HIGH;   // INPUT_PULLUP: LOW=닫힘
  String doorState = doorOpen ? "open" : "closed";
  DBGF("[DOOR] BL0303: %s\n", doorState.c_str());

  // JSON 직렬화
  StaticJsonDocument<256> doc;
  doc["height"]         = height;
  doc["angle"]          = angle;
  doc["tof_status"]     = height < 10 ? "error" : height < 50 ? "low" : "normal";
  doc["mmwave"]         = mmwave;
  doc["door"]           = doorState;
  doc["fall_candidate"] = fallCandidate;
  doc["fall_detected"]  = fallDetected;
  doc["servo_status"]   = "stopped";

  String payload;
  serializeJson(doc, payload);

  // POST
  HTTPClient http;
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/data";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2000);

  int code = http.POST(payload);
  if (code == 200) {
    DBGF("[DATA] 전송 완료 · height=%.1f · angle=%d · door=%s\n",
         height, angle, doorState.c_str());
    digitalWrite(LED_PIN, HIGH);
  } else {
    DBGF("[DATA] 전송 실패: HTTP %d — 서버 IP 확인 (%s:%d)\n",
         code, SERVER_IP, SERVER_PORT);
    digitalWrite(LED_PIN, LOW);
  }
  http.end();
}

// ============================================================
// SD카드 파일 구성 (/MP3 폴더, FAT32 포맷):
//   0001.mp3 → 평상시 배경 음악
//   0002.mp3 → 경고음
//   0003.mp3 → "낙상이 감지되었습니다."
//   0004.mp3 → "괜찮으십니까?"
//   0005.mp3 → "응답이 없으면 보호자에게 알림을 전송합니다."
//
// Serial Monitor 출력 예시:
//   [WiFi] 연결됨: 192.168.0.25
//   [Server] http://192.168.0.15:5000
//   [DATA] 전송 완료 · height=165.0 · angle=88 · door=closed
//   [DOOR] BL0303: open
//   [AUDIO] 재생: /MP3/0001.mp3
//   [BUTTON] 아케이드 버튼 눌림 → POST /button/press
// ============================================================

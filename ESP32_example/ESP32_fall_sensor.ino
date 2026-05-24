// ============================================================
// ESP32 낙상 감지 센서 노드 - Arduino 예시 코드
// ============================================================
// 준비물:
//   - ESP32 보드
//   - Arduino IDE + ESP32 보드 패키지
//   - WiFi 네트워크 (라즈베리파이와 같은 공유기에 연결)
//
// 설치 필요 라이브러리:
//   - Arduino IDE → 툴 → 라이브러리 관리
//   - "ArduinoJson" 검색 후 설치 (버전 6.x 이상)
//
// 사용 방법:
//   1. WIFI_SSID, WIFI_PASSWORD, SERVER_IP 를 본인 환경에 맞게 수정
//   2. ESP32 보드에 업로드
//   3. 시리얼 모니터 열기 (속도: 115200)
//   4. 라즈베리파이 Flask 서버가 실행 중이어야 수신됨
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>   // JSON 직렬화 라이브러리

// ── 설정값 (본인 환경에 맞게 수정하세요) ──────────────────
const char* WIFI_SSID     = "공유기이름";          // WiFi SSID
const char* WIFI_PASSWORD = "WiFi비밀번호";        // WiFi 비밀번호
const char* SERVER_IP     = "192.168.0.xxx";       // 라즈베리파이 IP
const int   SERVER_PORT   = 5000;
const int   POST_INTERVAL = 3000;                  // 전송 주기 (ms)

// ── 시뮬레이션용 변수 (실제 센서로 교체하세요) ────────────
// 아래 값을 직접 바꾸거나 실제 센서 읽기 코드로 교체하면 됩니다.
float  sim_height        = 165.0;
int    sim_angle         = 88;
String sim_tof_status    = "normal";  // "normal" | "low" | "error"
bool   sim_mmwave        = true;
String sim_door          = "closed";  // "open" | "closed"
bool   sim_fall_candidate = false;
bool   sim_fall_detected  = false;
String sim_servo_status  = "stopped"; // "stopped" | "checking" | "alert"

// ── setup() : 1회 초기화 ──────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n====================================");
  Serial.println("  ESP32 낙상 감지 노드 시작");
  Serial.println("====================================");

  // WiFi 연결
  Serial.print("WiFi 연결 중: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry++;
    if (retry > 30) {
      // 30초 대기 후 재부팅 (WiFi 연결 실패 시)
      Serial.println("\nWiFi 연결 실패 - 재부팅합니다.");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi 연결 성공!");
  Serial.print("ESP32 IP 주소: ");
  Serial.println(WiFi.localIP());
  Serial.print("서버 주소: http://");
  Serial.print(SERVER_IP);
  Serial.print(":");
  Serial.println(SERVER_PORT);
  Serial.println("====================================\n");
}

// ── loop() : 반복 실행 ────────────────────────────────────
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // WiFi 끊김 감지 시 재연결 시도
    Serial.println("[경고] WiFi 연결 끊김 - 재연결 중...");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  // ── 실제 센서 읽기 자리 ──────────────────────────────
  // 여기에 ToF, mmWave, 문 센서 읽기 코드를 넣으세요.
  // 예시: sim_height = tof_sensor.readRange();
  //       sim_mmwave = mmwave_sensor.detected();
  //       sim_door   = door_switch.read() ? "open" : "closed";
  //
  // 지금은 시뮬레이션 값을 그대로 사용합니다.
  // ──────────────────────────────────────────────────────

  sendSensorData();
  delay(POST_INTERVAL);
}

// ── HTTP POST 전송 함수 ───────────────────────────────────
void sendSensorData() {
  // JSON 문서 생성 (256바이트면 충분)
  StaticJsonDocument<256> doc;
  doc["height"]         = sim_height;
  doc["angle"]          = sim_angle;
  doc["tof_status"]     = sim_tof_status;
  doc["mmwave"]         = sim_mmwave;
  doc["door"]           = sim_door;
  doc["fall_candidate"] = sim_fall_candidate;
  doc["fall_detected"]  = sim_fall_detected;
  doc["servo_status"]   = sim_servo_status;

  // JSON → 문자열 직렬화
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // HTTP 클라이언트 설정
  HTTPClient http;
  String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/data";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  Serial.print("[POST] ");
  Serial.print(url);
  Serial.print(" → ");
  Serial.println(jsonPayload);

  // POST 전송
  int httpCode = http.POST(jsonPayload);

  if (httpCode > 0) {
    // 성공
    String response = http.getString();
    Serial.print("[응답 ");
    Serial.print(httpCode);
    Serial.print("] ");
    Serial.println(response);
  } else {
    // 실패 (서버 미실행 or IP 오류)
    Serial.print("[오류] HTTP POST 실패: ");
    Serial.println(http.errorToString(httpCode));
    Serial.println("  → 라즈베리파이 IP와 Flask 서버 실행 여부를 확인하세요.");
  }

  http.end();
}

// ============================================================
// 테스트 시나리오 변경 방법:
//   setup() 끝부분에 아래처럼 값을 설정하면 됩니다.
//
//   // 낙상 의심 시나리오
//   sim_height        = 45.0;
//   sim_angle         = 35;
//   sim_tof_status    = "low";
//   sim_fall_candidate = true;
//   sim_servo_status  = "checking";
//
//   // 낙상 확정 시나리오
//   sim_height        = 12.0;
//   sim_angle         = 5;
//   sim_fall_candidate = true;
//   sim_fall_detected  = true;
//   sim_servo_status  = "alert";
// ============================================================

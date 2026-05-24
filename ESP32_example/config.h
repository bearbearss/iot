// ============================================================
// Smart Bath Guard — ESP32 설정 파일
// ============================================================
// ★ 이 파일만 수정하면 됩니다 ★
//
// 수정 순서:
//   1. WIFI_SSID      → 공유기 이름
//   2. WIFI_PASSWORD  → 공유기 비밀번호
//   3. SERVER_IP      → Raspberry Pi IP (터미널에서 `hostname -I` 로 확인)
//   4. SERVER_PORT    → Flask 포트 (기본 5000)
//
// Raspberry Pi IP 확인:
//   $ hostname -I
//   192.168.0.15 ← 이 값을 SERVER_IP 에 입력
// ============================================================

#pragma once

// ── WiFi 설정 ─────────────────────────────────────────────
#define WIFI_SSID       "공유기이름"       // ← 수정 필수
#define WIFI_PASSWORD   "WiFi비밀번호"    // ← 수정 필수

// ── Raspberry Pi 서버 ─────────────────────────────────────
#define SERVER_IP       "192.168.0.xxx"  // ← 수정 필수  (hostname -I 로 확인)
#define SERVER_PORT     5000

// ── 핀 번호 ───────────────────────────────────────────────
#define DOOR_PIN        27    // BL0303 자석 스위치 (INPUT_PULLUP)
#define LED_PIN         2     // 내장 LED (WiFi 상태 표시)
#define BUTTON_PIN      34    // 60mm 아케이드 버튼

// ── 타이머 ────────────────────────────────────────────────
#define DATA_INTERVAL_MS   3000   // 센서 데이터 전송 주기 (ms)
#define AUDIO_INTERVAL_MS   500   // 오디오 명령 폴링 주기 (ms)
#define WIFI_RETRY_MAX       20   // WiFi 재시도 횟수 (각 500ms)

// ── AP 모드 설정 (최초 부팅 시) ───────────────────────────
#define AP_SSID         "SmartBathroom_Setup"
#define AP_PASSWORD     "setup1234"

// ── 디버그 출력 ────────────────────────────────────────────
#define DEBUG_SERIAL    true   // false 로 바꾸면 Serial 출력 비활성화
#define DBG(x)   if(DEBUG_SERIAL) Serial.print(x)
#define DBGLN(x) if(DEBUG_SERIAL) Serial.println(x)
#define DBGF(...)if(DEBUG_SERIAL) Serial.printf(__VA_ARGS__)

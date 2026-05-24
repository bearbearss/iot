// ============================================================
// Smart Bath Guard — ESP32 최초 설정 (AP 모드 캡티브 포털)
// ============================================================
// 사용 방법:
//   1. 이 스케치를 ESP32에 업로드
//   2. ESP32 전원 ON → "SmartBathroom_Setup" WiFi 생성
//   3. 스마트폰으로 "SmartBathroom_Setup" 접속 (비번: setup1234)
//   4. 브라우저에서 192.168.4.1 접속
//   5. WiFi 이름/비밀번호/서버 IP 입력 후 저장
//   6. ESP32 재부팅 → Preferences 에 저장된 설정으로 Flask 연결
//   7. 설정 완료 후 ESP32_dfplayer.ino 로 교체
//
// 라이브러리 설치:
//   - WebServer (기본 포함)
//   - Preferences (기본 포함)
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// AP 모드 설정
#define AP_SSID     "SmartBathroom_Setup"
#define AP_PASSWORD "setup1234"

WebServer  server(80);
Preferences prefs;

// ── 저장된 설정 로드 ─────────────────────────────────────
struct Config {
  String wifi_ssid;
  String wifi_password;
  String server_ip;
  int    server_port;
} cfg;

void loadConfig() {
  prefs.begin("sbg", true);   // read-only
  cfg.wifi_ssid      = prefs.getString("ssid",     "");
  cfg.wifi_password  = prefs.getString("pass",     "");
  cfg.server_ip      = prefs.getString("srv_ip",   "");
  cfg.server_port    = prefs.getInt   ("srv_port",  5000);
  prefs.end();
}

void saveConfig(String ssid, String pass, String srv_ip, int srv_port) {
  prefs.begin("sbg", false);  // read-write
  prefs.putString("ssid",     ssid);
  prefs.putString("pass",     pass);
  prefs.putString("srv_ip",   srv_ip);
  prefs.putInt   ("srv_port", srv_port);
  prefs.end();
}


// ── 설정 페이지 HTML ─────────────────────────────────────
const char* PAGE_HTML = R"(
<!DOCTYPE html><html lang="ko"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Bath Guard 설정</title>
<style>
  body{background:#070c1a;color:#e2e8f0;font-family:sans-serif;
       display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;}
  .card{background:#0d1630;border:1px solid #1a2e4a;border-radius:16px;
        padding:32px 28px;width:100%;max-width:400px;box-shadow:0 20px 60px rgba(0,0,0,.5);}
  h1{color:#00d4ff;font-size:1.3rem;margin:0 0 6px}
  p{color:#475569;font-size:.85rem;margin:0 0 20px}
  label{display:block;font-size:.78rem;font-weight:700;color:#94a3b8;
        margin-bottom:5px;text-transform:uppercase;letter-spacing:.4px}
  input{width:100%;padding:11px 14px;background:#111e38;border:1px solid #243c5e;
        border-radius:8px;color:#e2e8f0;font-size:.9rem;box-sizing:border-box;
        margin-bottom:14px;outline:none;}
  input:focus{border-color:#00d4ff;}
  button{width:100%;padding:13px;background:linear-gradient(135deg,#007aff,#00d4ff);
         border:none;border-radius:10px;color:#fff;font-size:.95rem;
         font-weight:700;cursor:pointer;}
  .note{font-size:.75rem;color:#475569;text-align:center;margin-top:14px}
  .ok{background:#0d2818;border:1px solid rgba(0,255,136,.4);border-radius:8px;
      padding:12px;text-align:center;color:#00ff88;margin-bottom:16px}
</style></head><body>
<div class="card">
  <h1>🚿 Smart Bath Guard</h1>
  <p>WiFi 및 서버 설정을 입력하세요</p>
  __STATUS__
  <form method="POST" action="/save">
    <label>WiFi 이름 (SSID)</label>
    <input name="ssid" value="__SSID__" placeholder="공유기 이름" required/>
    <label>WiFi 비밀번호</label>
    <input name="pass" type="password" value="__PASS__" placeholder="비밀번호"/>
    <label>Raspberry Pi IP</label>
    <input name="srv_ip" value="__IP__" placeholder="192.168.0.15" required/>
    <label>Flask 포트</label>
    <input name="srv_port" type="number" value="__PORT__" placeholder="5000"/>
    <button type="submit">💾 저장 후 재부팅</button>
  </form>
  <div class="note">저장 후 ESP32가 재부팅되어 Flask 서버에 연결됩니다</div>
</div></body></html>
)";

String buildPage(bool saved = false) {
  String html = PAGE_HTML;
  html.replace("__SSID__",   cfg.wifi_ssid);
  html.replace("__PASS__",   cfg.wifi_password);
  html.replace("__IP__",     cfg.server_ip);
  html.replace("__PORT__",   String(cfg.server_port));
  html.replace("__STATUS__", saved
    ? "<div class='ok'>✅ 저장 완료 — 3초 후 재부팅됩니다</div>"
    : "");
  return html;
}

// ── 라우트 핸들러 ─────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildPage());
}

void handleSave() {
  String ssid  = server.arg("ssid");
  String pass  = server.arg("pass");
  String srvIp = server.arg("srv_ip");
  int    port  = server.arg("srv_port").toInt();
  if (port == 0) port = 5000;

  saveConfig(ssid, pass, srvIp, port);
  cfg.wifi_ssid = ssid; cfg.wifi_password = pass;
  cfg.server_ip = srvIp; cfg.server_port = port;

  server.send(200, "text/html; charset=utf-8", buildPage(true));
  Serial.printf("[SAVE] SSID=%s  SERVER=%s:%d\n",
                ssid.c_str(), srvIp.c_str(), port);
  delay(3000);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302);
}


// ════════════════════════════════════════════════════════
// setup()
// ════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println("\n[AP Mode] Smart Bath Guard 초기 설정");

  loadConfig();

  // 저장된 WiFi가 있으면 STA 모드 시도
  if (cfg.wifi_ssid.length() > 0) {
    Serial.printf("[WiFi] 저장된 설정으로 연결 시도: %s\n", cfg.wifi_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
      delay(500); Serial.print(".");
      retry++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\n[WiFi] 연결됨: %s — 메인 펌웨어로 교체하세요\n",
                    WiFi.localIP().toString().c_str());
      // 실제 운용 시에는 여기서 ESP32_dfplayer.ino 로직을 호출하거나
      // SPIFFS 플래그로 메인 루프로 전환할 수 있습니다.
      return;
    }
    Serial.println("\n[WiFi] 연결 실패 — AP 모드 진입");
  }

  // AP 모드
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[AP] SSID: %s  비번: %s\n", AP_SSID, AP_PASSWORD);
  Serial.printf("[AP] 접속 주소: http://%s\n",
                WiFi.softAPIP().toString().c_str());

  server.on("/",      handleRoot);
  server.on("/save",  HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[AP] 웹서버 시작 — 스마트폰으로 SmartBathroom_Setup 접속");
}

// ════════════════════════════════════════════════════════
// loop()
// ════════════════════════════════════════════════════════
void loop() {
  server.handleClient();
}

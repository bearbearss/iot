# 🚿 스마트 욕실 낙상 감지 대시보드

> ESP32 센서 노드 → Raspberry Pi 5 Flask 서버 → Cloudflare Tunnel → 보호자 외부 접속

---

## 시스템 구조

```
[욕실 내부]                [욕실 외부]               [인터넷]
┌─────────────┐   HTTP    ┌──────────────────┐   Tunnel  ┌─────────────────────┐
│   ESP32     │  ──POST──▶│  Raspberry Pi 5  │ ─────────▶│  Cloudflare Edge    │
│             │           │  Flask (port 5000)│           │  (자동 HTTPS)        │
│  · ToF 센서  │           │                  │           └──────────┬──────────┘
│  · mmWave   │           │  · 데이터 수신     │                      │ HTTPS
│  · 문 센서   │           │  · 상태 판단      │                      ▼
│  · 서보모터  │           │  · 로그 저장      │           ┌─────────────────────┐
└─────────────┘           │  · 웹 대시보드     │           │   보호자 스마트폰    │
                          └──────────────────┘           │  https://xxxx.xxx   │
                                                          └─────────────────────┘
```

---

## 주요 기능

| 기능 | 설명 |
|------|------|
| 실시간 센서 대시보드 | 1초 간격으로 센서 상태 갱신 |
| 낙상 상태 판단 | 정상 / 낙상 의심 / 낙상 감지 / 센서 오류 |
| 보호자 로그인 | 접근 코드 입력 후 24시간 세션 유지 |
| 로그 기록 | 최근 20개 이벤트 자동 저장 |
| 테스트 모드 | ESP32 없이 버튼 클릭으로 시뮬레이션 |
| 모바일 반응형 | 스마트폰에서도 최적화된 UI |

---

## 폴더 구조

```
smart_bathroom_dashboard/
├── app.py                  # Flask 서버 (API + 로그인)
├── requirements.txt        # 패키지 목록
├── .env.example            # 환경 변수 예시 (복사 후 .env 생성)
├── .gitignore
├── templates/
│   ├── login.html          # 보호자 로그인 페이지
│   └── index.html          # 메인 대시보드
├── static/
│   ├── style.css           # UI 스타일
│   └── script.js           # 실시간 폴링 + 테스트 버튼
└── ESP32_example/
    └── ESP32_fall_sensor.ino
```

---

## Raspberry Pi 실행 방법

### 1. 프로젝트 클론

```bash
git clone https://github.com/<your-id>/smart_bathroom_dashboard.git
cd smart_bathroom_dashboard
```

### 2. 환경 변수 설정

```bash
cp .env.example .env
nano .env
```

`.env` 파일 내용 (본인 값으로 변경):
```
ACCESS_CODE=원하는접근코드
SECRET_KEY=긴랜덤문자열  # python3 -c "import secrets; print(secrets.token_hex(32))"
```

### 3. 가상환경 + 패키지 설치

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### 4. 서버 실행

```bash
python app.py
```

> 서버가 시작되면 `http://0.0.0.0:5000` 에서 수신 대기

---

## GitHub 업로드 방법

```bash
git init
git add .
git commit -m "first commit"
git branch -M main
git remote add origin https://github.com/<your-id>/smart_bathroom_dashboard.git
git push -u origin main
```

> `.env` 파일은 `.gitignore`에 포함되어 자동으로 제외됩니다.

---

## Cloudflare Tunnel 연결

> **주의**: 이 프로젝트는 Flask 서버이므로 **Cloudflare Tunnel**을 사용합니다.  
> Cloudflare Pages(정적 사이트용)와 다릅니다.

### Raspberry Pi에 cloudflared 설치

```bash
# ARM64 (Raspberry Pi 5)
wget https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64.deb
sudo dpkg -i cloudflared-linux-arm64.deb
```

### 빠른 임시 터널 (테스트용)

```bash
cloudflared tunnel --url http://localhost:5000
```

터미널에 출력되는 URL을 보호자에게 공유하면 됩니다:
```
https://xxxx-xxxx.trycloudflare.com  ← 이 주소 공유
```

### 고정 도메인 터널 (운영용)

Cloudflare 대시보드 → Zero Trust → Networks → Tunnels에서 설정.  
설정 완료 후 `https://your-domain.com` 형태의 고정 URL로 접속 가능.

---

## 외부 접속 흐름

```
보호자 스마트폰 브라우저
  → https://xxxx.trycloudflare.com/login   (로그인 페이지)
  → 접근 코드 입력
  → https://xxxx.trycloudflare.com/        (대시보드)
  → 1초마다 자동 갱신
```

---

## API 엔드포인트

| 경로 | 메서드 | 인증 | 설명 |
|------|--------|------|------|
| `/login` | GET/POST | 불필요 | 보호자 로그인 |
| `/logout` | GET | 불필요 | 로그아웃 |
| `/` | GET | **필요** | 대시보드 화면 |
| `/api/latest` | GET | **필요** | 최신 센서 상태 |
| `/api/logs` | GET | **필요** | 최근 20개 로그 |
| `/data` | POST | 불필요 | ESP32 데이터 수신 |
| `/test/normal` | POST | **필요** | 정상 테스트 주입 |
| `/test/warning` | POST | **필요** | 낙상 의심 테스트 |
| `/test/fall` | POST | **필요** | 낙상 감지 테스트 |

---

## curl 테스트 명령어

```bash
# ESP32 역할 - 센서 데이터 전송
curl -X POST http://localhost:5000/data \
  -H "Content-Type: application/json" \
  -d '{"height":23.5,"angle":80,"tof_status":"low","mmwave":true,"door":"closed","fall_candidate":true,"fall_detected":false,"servo_status":"stopped"}'

# 최신 상태 확인
curl http://localhost:5000/api/latest
```

---

## 오류 체크리스트

| 증상 | 확인 사항 |
|------|-----------|
| 페이지가 안 열림 | `python app.py` 실행 중인지 확인 |
| 로그인이 안 됨 | `.env`의 `ACCESS_CODE` 확인 |
| ESP32 전송 실패 | 라즈베리파이 IP 주소와 같은 WiFi 여부 확인 |
| Cloudflare 연결 안 됨 | `cloudflared` 실행 중인지 확인 |
| 세션이 자꾸 끊김 | `.env`의 `SECRET_KEY`가 재시작 시 바뀌지 않는지 확인 |

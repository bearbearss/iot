# 스마트 욕실 낙상 감지 대시보드 - 실행 가이드

## 1. 라즈베리파이에서 서버 실행

```bash
# 프로젝트 폴더로 이동
cd smart_bathroom_dashboard

# 가상환경 생성 (최초 1회)
python3 -m venv venv

# 가상환경 활성화
source venv/bin/activate

# 패키지 설치 (최초 1회)
pip install -r requirements.txt

# 서버 실행
python app.py
```

> 성공 시 출력:
> ```
> =====================================================
>   스마트 욕실 낙상 감지 서버 시작
>   접속: http://0.0.0.0:5000
>   같은 WiFi 기기: http://<라즈베리파이 IP>:5000
> =====================================================
> ```

---

## 2. 라즈베리파이 IP 확인 방법

```bash
hostname -I
# 또는
ip addr show | grep "inet " | grep -v 127
```

예시 출력: `192.168.0.42` → 이 IP를 사용

---

## 3. 브라우저에서 접속

| 기기 | 접속 주소 |
|------|-----------|
| 라즈베리파이 본체 | http://localhost:5000 |
| 같은 WiFi의 노트북/폰 | http://192.168.0.42:5000 |

---

## 4. ESP32 없이 테스트하는 방법

### 방법 A: 웹 화면 버튼 클릭
브라우저에서 대시보드를 열고 하단 테스트 버튼 3개를 클릭합니다.
- ✅ 정상 상태 테스트
- ⚠️ 낙상 의심 테스트  
- 🆘 낙상 감지 테스트

### 방법 B: curl 명령어

```bash
# 정상 상태
curl -X POST http://192.168.0.42:5000/test/normal

# 낙상 의심
curl -X POST http://192.168.0.42:5000/test/warning

# 낙상 감지
curl -X POST http://192.168.0.42:5000/test/fall

# 직접 JSON 데이터 전송 (ESP32 흉내)
curl -X POST http://192.168.0.42:5000/data \
  -H "Content-Type: application/json" \
  -d '{"height":23.5,"angle":80,"tof_status":"low","mmwave":true,"door":"closed","fall_candidate":true,"fall_detected":false,"servo_status":"stopped"}'
```

---

## 5. ESP32 연결 방법

1. `ESP32_example/ESP32_fall_sensor.ino` 파일을 Arduino IDE에서 열기
2. 상단 3개 변수 수정:
   ```cpp
   const char* WIFI_SSID     = "공유기이름";
   const char* WIFI_PASSWORD = "WiFi비밀번호";
   const char* SERVER_IP     = "192.168.0.42";  // 라즈베리파이 IP
   ```
3. ESP32에 업로드
4. 시리얼 모니터(115200) 열어서 전송 로그 확인

---

## 6. 오류 체크리스트

| 증상 | 확인 사항 |
|------|-----------|
| 페이지가 안 열림 | `python app.py` 실행 중인지, 포트 5000이 방화벽에 막혀있지 않은지 확인 |
| "연결 오류" 배지 표시 | Flask 서버 재시작, 브라우저 콘솔(F12) 오류 확인 |
| ESP32가 전송 실패 | IP 주소 오타 확인, 라즈베리파이와 같은 WiFi인지 확인 |
| `ModuleNotFoundError: flask` | `pip install -r requirements.txt` 재실행 |
| 가상환경 비활성화 후 오류 | `source venv/bin/activate` 다시 실행 |

---

## 7. 향후 확장

### AI 판단 추가
`app.py`의 `determine_status()` 함수에 ML 모델 호출 추가:
```python
def determine_status(data):
    # 기존 규칙 기반 로직 유지 또는 교체
    prediction = my_ai_model.predict(data)
    return prediction
```

### SQLite 로그 저장
```bash
pip install flask-sqlalchemy
```
`app.py`에 SQLAlchemy 모델 추가 후 `save_data()`에서 `db.session.add()` 호출.
현재 `log_history` deque는 그대로 두고 DB를 병행 저장하면 됩니다.

### 알림 기능
`save_data()` 내부에서 `fall_detected` 시 Telegram Bot API 또는 이메일 발송:
```python
if data.get("fall_detected"):
    send_telegram_alert("낙상이 감지되었습니다!")
```

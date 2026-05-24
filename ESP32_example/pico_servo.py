# ============================================================
# Smart Bath Guard — Raspberry Pi Pico 서보모터 제어
# ============================================================
# 역할: 서보모터를 35°~140° 범위에서 왕복 회전
#       현재 각도를 UART로 ESP32에 전송
#
# 배선:
#   서보모터 VCC(빨강) → VBUS (5V) 또는 외부 5V
#   서보모터 GND(갈색) → GND
#   서보모터 PWM(주황) → GP15
#
#   Pico TX (GP0) → ESP32 GPIO25 (PICO_RX_PIN)
#   Pico RX (GP1) → ESP32 GPIO26 (PICO_TX_PIN)
#   Pico GND      → ESP32 GND  ← 반드시 연결!
#
# 업로드:
#   Thonny IDE 에서 이 파일을 Pico 에 main.py 로 저장
#   → 전원 켤 때 자동 실행됨
# ============================================================

from machine import Pin, PWM, UART
import time

# ── 설정 ──────────────────────────────────────────────────
SERVO_PIN  = 15       # 서보 PWM 핀 (GP15)
UART_ID    = 0        # UART0 사용 (TX=GP0, RX=GP1)
BAUD_RATE  = 9600

ANGLE_MIN  = 35       # 스캔 시작 각도 (도)
ANGLE_MAX  = 140      # 스캔 끝 각도 (도)
SCAN_STEP  = 5        # 한 번에 몇 도씩 이동 (작을수록 촘촘)
SCAN_DELAY = 0.35     # 각도마다 정지 시간 (초)

# ── 초기화 ────────────────────────────────────────────────
servo = PWM(Pin(SERVO_PIN))
servo.freq(50)   # 서보는 50Hz 제어

uart = UART(UART_ID, baudrate=BAUD_RATE, tx=Pin(0), rx=Pin(1))

# ── 각도 → PWM 펄스 변환 ──────────────────────────────────
def set_angle(deg):
    """
    SG90 기준 펄스폭:
      0°   = 500 µs
      90°  = 1500 µs
      180° = 2500 µs
    duty_u16: 0~65535 (= 0~100%)
    주기 20ms (50Hz) 기준으로 계산
    """
    deg = max(0, min(180, deg))
    us   = 500 + (2000 * deg // 180)
    duty = int(us * 65535 // 20000)
    servo.duty_u16(duty)

# ── 현재 각도를 ESP32 로 전송 ─────────────────────────────
def send_angle(deg, status="scanning"):
    msg = f"ANGLE:{deg}\nSTATUS:{status}\n"
    uart.write(msg.encode())

# ── 서보 중립 위치로 이동 후 대기 ─────────────────────────
set_angle(90)
time.sleep(0.5)

print("Pico 서보 제어 시작")
send_angle(90, "ready")

# ── 메인 루프: 35°~140° 왕복 ──────────────────────────────
current = ANGLE_MIN
direction = 1   # 1 = 증가 방향, -1 = 감소 방향

while True:
    # 서보 이동
    set_angle(current)

    # ESP32 로 현재 각도 전송
    send_angle(current, "scanning")

    print(f"각도: {current}°")

    time.sleep(SCAN_DELAY)

    # 다음 각도 계산
    current += SCAN_STEP * direction

    # 경계 처리 (왕복)
    if current >= ANGLE_MAX:
        current   = ANGLE_MAX
        direction = -1
        send_angle(current, "reversing")
    elif current <= ANGLE_MIN:
        current   = ANGLE_MIN
        direction = 1
        send_angle(current, "reversing")

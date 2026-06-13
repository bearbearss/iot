from machine import Pin, PWM, UART
import time

# ESP32와 UART 통신
# Pico UART0: GP0 = TX, GP1 = RX
uart = UART(0, baudrate=115200)

# 서보모터 Signal 핀: GP14
servo = PWM(Pin(14))
servo.freq(50)

# 서보 각도 이동 함수
def set_angle(angle):
    if angle < 0:
        angle = 0
    if angle > 180:
        angle = 180

    # SG90/MG90S 기준 대략값
    min_duty = 1000
    max_duty = 9000

    duty = int(min_duty + (angle / 180) * (max_duty - min_duty))
    servo.duty_u16(duty)

# 시작 위치
set_angle(90)
time.sleep(1)

print("Pico Servo Controller Start")
print("ESP32 각도 명령 대기 중...")

while True:
    data = uart.read()

    if data:
        try:
            text = data.decode().strip()
            print("받은 데이터:", text)

            # 숫자만 추출
            num = ""
            for ch in text:
                if ch >= "0" and ch <= "9":
                    num += ch

            if num != "":
                angle = int(num)
                print("서보 이동 각도:", angle)
                set_angle(angle)

        except Exception as e:
            print("에러:", e)

    time.sleep(0.05)


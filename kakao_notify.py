import json
import os
import requests

TOKEN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "kakao_token.json")

REST_API_KEY  = "64006507854318eb1b17b347aeecc31f"
CLIENT_SECRET = "s6KZqVuXeObZDZbMuQL4OByfG2qOiVLo"

ALERT_TEMPLATE = {
    "object_type": "text",
    "text": (
        "사용자가 욕실 내에서 일정 시간 이상 움직임이 감지되지 않아 "
        "이상 상황이 의심됩니다.\n사용자 상태를 확인해 주세요."
    ),
    "link": {
        "web_url":        "https://developers.kakao.com",
        "mobile_web_url": "https://developers.kakao.com",
    },
    "button_title": "확인하기",
}


def _load_tokens() -> dict:
    with open(TOKEN_FILE, "r") as fp:
        return json.load(fp)


def _save_tokens(tokens: dict):
    with open(TOKEN_FILE, "w") as fp:
        json.dump(tokens, fp)


def _refresh_access_token() -> str:
    tokens = _load_tokens()
    resp = requests.post(
        "https://kauth.kakao.com/oauth/token",
        data={
            "grant_type":    "refresh_token",
            "client_id":     REST_API_KEY,
            "client_secret": CLIENT_SECRET,
            "refresh_token": tokens["refresh_token"],
        },
    )
    new = resp.json()
    tokens["access_token"] = new["access_token"]
    if "refresh_token" in new:
        tokens["refresh_token"] = new["refresh_token"]
    _save_tokens(tokens)
    return tokens["access_token"]


def send_fall_alert() -> bool:
    """
    낙상 위험 카카오 메시지 전송.
    토큰 만료 시 자동 갱신 후 재시도.
    반환값: 전송 성공 여부
    """
    if not os.path.exists(TOKEN_FILE):
        print("[KAKAO] 토큰 파일 없음 — kakao_token.json 을 먼저 생성하세요.")
        return False

    try:
        tokens = _load_tokens()
        access_token = tokens.get("access_token", "")

        def _post(token: str):
            return requests.post(
                "https://kapi.kakao.com/v2/api/talk/memo/default/send",
                headers={"Authorization": "Bearer " + token},
                data={"template_object": json.dumps(ALERT_TEMPLATE)},
            )

        res = _post(access_token)
        body = res.json()

        # 토큰 만료(-401) 시 갱신 후 재시도
        if body.get("code") == -401:
            print("[KAKAO] 토큰 만료 — 갱신 중...")
            access_token = _refresh_access_token()
            res = _post(access_token)
            body = res.json()

        if body.get("result_code") == 0:
            print("[KAKAO] 보호자 알림 전송 성공")
            return True
        else:
            print(f"[KAKAO] 전송 실패: {body}")
            return False

    except Exception as e:
        print(f"[KAKAO] 오류: {e}")
        return False

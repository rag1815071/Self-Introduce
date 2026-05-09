import os
import json
import ssl
import datetime
from confluent_kafka import Producer
import paho.mqtt.client as mqtt


def _load_env_basic(path=".env"):
    if not os.path.exists(path):
        return
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip()
            # 따옴표 제거
            if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                val = val[1:-1]
            os.environ[key] = val

# .env 자동 로드
_load_env_basic(".env")

# ========== Kafka 설정 ==========
KAFKA_BROKER = os.getenv("KAFKA_BROKER", "127.0.0.1:9092")
KAFKA_TOPIC  = os.getenv("KAFKA_TOPIC",  "solar-data")

producer = Producer({'bootstrap.servers': KAFKA_BROKER})

# ========== MQTT 설정 ==========
MQTT_BROKER = "YOUR HiveMQ URL"
MQTT_PORT = 8883 
MQTT_USER = "ESP32_for_solar"
MQTT_PASSWORD = "YOUR_MQTT_PASSWORD"
MQTT_TOPIC = "solar-data"

# -------- 발전량 임계값 ------
ENERGY_THRESHOLD = float(os.getenv("ENERGY_THRESHOLD", "500.0"))

# 장치별 상태 관리 : 딕셔너리 사용
device_state = {}

# ========= Kafka 전송 콜백 =========
def delivery_report(err, msg):
    if err is not None:
        print(f"[Kafka] 전송 실패: {err}")
    else:
        print(f"[Kafka] 전송 성공: {msg.value().decode('utf-8')}")

# ========= MQTT 이벤트 =========
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[HiveMQ] 연결 성공")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"[HiveMQ] 연결 실패: {rc}")

def on_message(client, userdata, msg):
    global device_state

    try:
        payload = json.loads(msg.payload.decode())
        device_id = payload.get("device_id")
        power_str = payload.get("power")
        timestamp_str = payload.get("timestamp")

        if not device_id or not timestamp_str or power_str in [None, "NaN"]:
            return 

        try:
            power = float(power_str)
            if power < 0 or power > 10000:
                print(f"[Outlier]: {power}")
                return
        except ValueError:
            print(f"[TypeError]: {power_str}")
            return

        timestamp_sec = datetime.datetime.fromisoformat(timestamp_str).timestamp()

        # 장치별 상태 초기화
        if device_id not in device_state:
            device_state[device_id] = {"total_energy": 0.0, "last_time": timestamp_sec}
            print(f"[{device_id}] 첫 수신 시간: {timestamp_str}")
            return

        last_time = device_state[device_id]["last_time"]
        interval = timestamp_sec - last_time
        if interval < 0:
            return

        device_state[device_id]["last_time"] = timestamp_sec
        current_energy = ((power) * (interval / 3600.0)) * 1000000
        device_state[device_id]["total_energy"] += current_energy

        total_energy = device_state[device_id]["total_energy"]
        print(f"[{device_id}] 현재 누적 발전량: {total_energy:.3f} Wh (interval={interval:.2f}s)")

        if total_energy >= ENERGY_THRESHOLD:
            #total_energy = total_energy*1000
            message = {
                "device_id": device_id,
                "timestamp": timestamp_str,
                "total_energy": round(total_energy, 3)
            }
            producer.produce(KAFKA_TOPIC, json.dumps(message), callback=delivery_report)
            producer.poll(0)
            #total_energy = total_energy/1000
            device_state[device_id]["total_energy"] = 0
            print(f"[{ENERGY_THRESHOLD}Wh 생산 성공]: {json.dumps(message)}")

    except Exception as e:
        print(f"[ESP32 Error] {e}")

# ========= MQTT 클라이언트 =========
client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

client.tls_set(cert_reqs=ssl.CERT_NONE)
client.tls_insecure_set(True)

print("[HiveMQ to Kafka Bridge Running...]")
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_forever()

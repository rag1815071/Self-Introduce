# Hardware Setup Guide

해당 README.md는 `Hardware/` 디렉토리의 **ESP32 기반 발전량 데이터 송신** 및 **MQTT - Kafka 데이터 전송 브리지(Python)** 실행 방법을 안내합니다.

- **대상 디렉토리:** `Hardware/`
> 
> 
- **구성 요소:** `ESP32/` `MQTT_Kafka_Bridge/`
> 

---

## Table of Contents

- [1. Overview](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [2. Required Ports](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [3. ESP32 Setup (Arduino)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [3.1 Environment / IDE](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [3.2 Run Conditions](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [3.3 Run ESP32](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [4. MQTT → Kafka Bridge (Python)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [4.1 Prerequisites](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [4.2 Run Bridge](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [Troubleshooting](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)

---

## 1. Overview

Hardware 모듈은 아래와 같이 동작합니다.
```
1. ESP32가 발전량 데이터를 수집
2. MQTT(HiveMQ)로 publish
3. Python Bridge가 MQTT를 subscribe
4. 위 데이터를 최종적으로 Kafka 토픽에 전달
```
---

## 2. Required Ports
| Port | Component | Purpose | Note |
| --- | --- | --- | --- |
| 1883 | MQTT | MQTT 일반 연결 | HiveMQ 클라우드 사용 시 보통 1883 또는 TLS 포트 사용 |
| 8883 | MQTT (TLS) | MQTT TLS 연결 | HiveMQ 설정에 따라 사용 |
| 9092 | Kafka Broker | Kafka produce/consume | 브로커 주소(IP/도메인) 필요 |

> ※ 실제 포트번호는 HiveMQ/브로커 설정에 따라 달라질 수 있습니다.

---

## 3. ESP32 Setup (Arduino)

### 3.1 Environment

### 1) Arduino IDE 설치

- 설치 링크: https://www.arduino.cc/en/software

### 2) USB 드라이버 설치 (Silabs VCP)

- 설치 링크: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads

### 3) ESP32 Board Package 설치

Arduino IDE에서 아래 메뉴로 진입합니다.

- `Tools` → `Board` → `Board Manager`
- 검색창에 `ESP32` 입력 후 ESP32 보드 패키지 설치

### 4) 보드 선택

- `Tools` → `Board` → `esp32` → `ESP32 Dev Module`

---

### 3.2 Run Conditions

- **유효한 HiveMQ URL**이 필요합니다.
    - (예: `xxxx.s1.eu.hivemq.cloud`)
- **유효한 WIFI 접속정보**(SSID, Passward)가 필요합니다.

---

### 3.3 Run ESP32
```
1. ESP32를 USB 포트에 연결
2. Arduino IDE 좌측 상단의 화살표(Upload/Run) 아이콘을 클릭하여 빌드 및 업로드
3. IDE 하단의 Serial Monitor를 열어 실행 로그 및 전송 상태를 확인

※ Serial Monitor의 baud rate는 코드에 정의된 값과 일치하는 것을 권장합니다. 
```
---

## 4. MQTT - Kafka Bridge (Python)

### 4.1 Environment

- Python 설치 (3.x 권장)
    - 설치 링크: https://www.python.org/downloads/
- Kafka 브로커 주소(IP/도메인)가 **프로그램에서 인식 가능한 방식으로** 제공되어야 합니다.
    - 예: 코드 내부 설정 / 환경변수 / `.env` / config 파일 등(프로젝트 구성에 따름)

---

### 4.2 Run Bridge

브리지 디렉토리의 루트 위치에서 아래 명령으로 실행합니다.

```bash
python mqtt_to_kafka.py

```

## Troubleshooting

### ESP32 업로드가 실패하는 경우

- USB 드라이버 설치 여부 확인(Silabs VCP)
- Arduino IDE에서 보드/포트 선택 확인
    - `Tools` → `Board` / `Tools` → `Port`
- 케이블이***충전 전용***인지 확인 (데이터 전송 가능한 케이블 권장)

### Serial Monitor에 대한 로그 출력이 실패하는 경우

- baud rate 설정 확인(코드와 동일해야 함)
- 포트가 다른 프로그램에 점유되어 있지 않은지 확인

### Bridge와 Kafka의 연결이 실패하는 경우

- Kafka 브로커 주소/포트(기본 9092) 확인
- 브로커가 외부 접속 가능하게 열려 있는지 확인(방화벽/보안그룹/리스너 설정)
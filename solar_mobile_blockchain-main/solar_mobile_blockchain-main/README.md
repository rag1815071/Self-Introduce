
# Solar-Energy-Driven Mobile Blockchain Platform
**태양광 발전량 데이터**를 기반으로 모바일 라이트 노드들에 대한 참여를 유도하고, Kafka Broker-Proxy Node(릴레이 서버)-Cosmos SDK 기반의 Full Node를 결합해 **데이터생성/검증/보상**일련의 자동화된 파이프라인으로 이어지는 모바일 블록체인 플랫폼입니다.

> 하위 프로젝트별 상세 실행 방법은 각 디렉토리의 `README.md`를 참고하세요.

---

## 1. 프로젝트 개요

### 1) 문제배경
기존 블록체인 참여는 고성능 연산 장치(Proof Of Work) 또는 큰 경제적 자본(Proof of Steak)을 요구하며 이는 일반 사용자 들에게 **높은 진입 장벽**으로 작용합니다. 더불어 이와 같은 채굴 알고리즘과 행위는 **전력 낭비·자본 집중화 등 사회적 역효과**를 유발할 수 있습니다.  
또한 암호화폐는 내재적 가치 기준의 부재로 인해 **투기적 수단**으로 변질 될 위험이 존재합니다.

### 2) 목표
- **모바일 장치에서도 운용 가능한 라이트 노드 중심**의 네트워크 생태계 구축을 통해 참여의 진입 장벽을 낮춥니다. 
- **태양광 발전 데이터 기반의 채굴/보상 구조**로 친환경 에너지 참여를 유도하고, **지구온난화** 라는 사회적 문제 해결에 기여할 수 있는 방향을 탐구합니다.  
- **REC와 토큰 경제를 연계**하여 암호화폐에 내재적 가치를 부여하는 구조를 설계/구현합니다. 
```
※ REC는 Renewable Energy Certificate의 약자로서 '신재생에너지 공급인증서'를 의미합니다. 1MWh의 신재생에너지 발전량당 1REC가 발급되며, 이를 대형 발전사들이 의무적으로 구매해 신재생에너지 공급의무(RPS)를 이행하는 데 사용되거나, 현물/계약 시장에서 거래하여 현금화할 수 있습니다
```
---

## 2. 모듈의 역할

- **Hardware (ESP32 + PZEM004T)**
  - 태양광 발전량 계측 결과를 MQTT(HiveMQ)에게 publish한다.  
  - `device/{device_id}/energy` 기반의 메세지 형태로 디바이스 단위 구분을 고려  

- **MQTT ↔ Kafka Bridge**
  - MQTT를 통해 전송받은 발전량 메시지를 Kafka Broker의 토픽으로 전송

- **Light Node(gomobile) + Mobile App(Flutter)**
  - 라이트 노드와 1대1로 매핑된 발전장치로 부터 데이터를 수신하고 모바일 기기를 통해 해시 생성 및 서명(Ed25519)을 수행하여 그 결과를 Kafka Broker에 전송한다.  
  - 태양광 발전량 데이터,서명,보상 등의 결과를 UI에 반영한다.  

- **Proxy Node (Off-chain Relay)**
  - 라이트 노드 등록/인증, 태양광 발전장치(device_id)와 라이트노드 간의 1대1 매핑, 데이터/보상/서명 정보 중계, 공정한 블록 생성자 선출 등 전반적인 릴레이를 수행한다.
  - 기상청 일사량 데이터를 API를 통해 수집하고, 이를 매개 변수로 하여 서명 보상 점수 산출에 활용한다.  

- **Full Node (Cosmos SDK) + Fullnode Bridge**
  - Proxy Node로 부터 라이트 노드의 수집/집계된 서명 데이터를 전달 받는다.
  - Full Node는 검증/보상/REC 발급·소각을 수행하고 상태를 커밋한다.

---

## 3. 메시지 구성

### Topic
- 발전량/서명: `solar-data`, `light-vote-topic`
- 보상/조회: `balance-topic`, `tx-hash-topic`, `request-tx-hash-topic`, `result-tx-hash-topic`
- 매핑/관리: `device-address-request-topic`, `device-address-response-topic`, `request-user-count-topic`, `user-count-topic`
- 부가 기능: `request-collaterals-topic`, `collaterals`, `request-burn-topic`, `result-burn-topic`, `rec-price-topic`, `block-creator`, `contributors` 등

### 예시(요약)

#### 1) MQTT(HiveMQ) → Proxy Node: 발전량 데이터
```json
{
  "device_id": "pzem-001",
  "timestamp": "1735906110",
  "power": "25.4"
}
```
#### 2) Proxy Node → Kafka
```json
{
  "device_id": "pzem-001",
  "timestamp": "2025-02-03T12:11:20Z",
  "total_energy": 19321
}
```
#### 3) Light Node → Kafka: 서명 메시지
```json
{
  "hash": "BASE64_HASH",
  "original": {
    "device_id": "esp32-sim-01",
    "timestamp": "2025-02-03T12:11:20Z",
    "total_wh": 19321,
    "seq": 102
  },
  "pubkey": "BASE64_PUBKEY",
  "signature": "BASE64_SIGNATURE"
}
```
#### 4) Full Node 검증 결과 → Kafka
```json
{
  "hash": "BASE64_HASH",
  "ok": true,
  "reward": 0.052
}
```
#### 5) Full Node → Proxy Node: 참여자 발전량 기여도
```json
{
  "fullnode_id": "fullnode-1",
  "contributors": [
    { "address": "cosmos1aaa...", "energy_kwh": 1.25 },
    { "address": "cosmos1bbb...", "energy_kwh": 0.73 }
  ]
}
```
#### 6) Proxy Node → Full Node: 블록 생성자 선정 결과
```json
{
  "creator": "cosmos1aaa...",
  "contribution": 1.25,
  "fullnode_id": "fullnode-1"
}
```
#### 7) 태양광 발전 장치(device_id) - 라이트 노드: 1대1 매핑
```json
{
  "device_id": "pzem-001",
  "sender_id": "fullnode-1"
}
```
#### 8) 라이트노드 → Proxy Node: Tx 해시 조회 요청/응답
- 요청
```json
{
  "user_address": "cosmos1aaa..."
}
```
- 응답
```json
{
  "address": "cosmos1aaa...",
  "hash": "A1B2C3D4..."
}
```
#### 9) 라이트노드 → Full Node: 잔액 조회 요청/응답
- 요청
```json
{
  "node_id": "lightnode-01",
  "user_address": "cosmos1aaa..."
}
```
- 응답
```json
{
  "node_id": "lightnode-01",
  "address": "cosmos1aaa...",
  "balance": "12345urec"
}
```
#### 10) 라이트노드 → Full Node: 소각(burn) 요청/응답
- 소각 요청
```json
{
  "address": "cosmos1aaa...",
  "coin": "1000ustable"
}
```
- 소각 결과
```json
{
  "address": "cosmos1aaa...",
  "status": true,
  "tx_hash": "ABCDEF1234...",
  "rec_records": [
    { "seq": 102, "deviceId": "pzem-001", "energyWh": 19321 }
  ],
  "rec_metas": [
    { "recId": "REC-2025-0001", "owner": "cosmos1aaa..." }
  ],
  "error_reason": ""
}
```
#### 11) 라이트노드 → Full Node: 담보(collateral) 요청/응답
- 담보 예치 요청
```json
{
  "address": "cosmos1aaa...",
  "coin": "1000ustable"
}
```
- 결과(tx hash 문자열)
```
"ABCDEF1234..."
```
## 🚀 About Team
- **Developer:** 김민혁(19), 송 산(20), 이수인(21)
- **Advisor:** 고윤민 교수님


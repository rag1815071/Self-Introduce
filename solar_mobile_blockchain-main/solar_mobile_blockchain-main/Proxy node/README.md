# Proxy Node Setup Guide

해잔 README.md는 **프록시 노드** 실행을 위한 환경 설정, 필수 도구(PostgreSQL/Kafka) 준비, 그리고 동작 확인 방법을 정리합니다.

프록시 노드는 **라이트 노드(모바일 어플리케이션)-Kafka-풀노드** 사이에서 메시지를 중계하고, 일부 데이터(사용자 정보/트랜잭션 등)를 **데이터 베이스(PostgreSQL)에 저장**하며, **블록 생성 및 보상계산** 등의 로직을 수행합니다.

---

## 목차

- [1. 구성요소 요약](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [2. 필수 포트 표](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [3. 사전 요구사항](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [4. 설정값(중요)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [5. Kafka 토픽 준비](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [6. PostgreSQL 초기화](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [7. 실행 방법](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [8. 동작 확인(Quick Test)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [9. 트러블슈팅](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [10. 알려진 이슈(필독)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)

---

## 1. 구성요소 요약

프록시 노드는 실행 시 아래 기능들이 함께에 실행됩니다.

- **HTTP API 서버 (port: 3001)**
    - `POST /connect` : 사용자/디바이스 등록 정보 저장
    - `POST /verify` : 가입 여부 확인
- **Kafka Consumers/Producers**
    - 태양광 발전장치-라이트노드 1:1 매핑 요청/응답
    - 서명 참여 노드 수(count) 요청/응답
    - 서명 참여자 보상 계산 및 점수 누적(vote_counter)
    - tx hash 저장 및 조회 요청 처리
- **PostgreSQL 저장**
    - userData (사용자/태양광 발전장치 정보)
    - solar_archive (tx hash archive)
    - vote_counter (서명자 점수/누적치)

---

## 2. 포트 정보

| 구분 | 포트 | 방향 | 설명 |
| --- | --- | --- | --- |
| Proxy Node HTTP API | 3001 | Inbound | `/connect`, `/verify` |
| Proxy Node Metrics | 9090 | Inbound | `GET /metrics` (Prometheus 포맷) |
| PostgreSQL | 5432 | Outbound | DB 저장/조회 |
| Kafka Broker | 9092 | Outbound | 메시지 소비/생산 (브로커 환경에 따라 다름) |
| Full Node RPC (Tendermint) | 26657 | Outbound | tx 조회용 RPC (환경에 따라 다름) |


---

## 3. 사전 요구사항

### 3.1 필수 설치

- **Go** (go.mod 기준: `go 1.23`, toolchain `go1.24.4`)
    - 네트워크 제한 환경에서 toolchain 자동 다운로드가 막히면:
        - `GOTOOLCHAIN=local` 설정 후 로컬 Go 버전으로 빌드하세요.
- **PostgreSQL**
- **Kafka Cluster** (또는 단일 브로커)

## 4. 설정값

프록시 노드의 주요 설정은 `config/config.go` 파일에 정의되어 있습니다.

### 4.1 Kafka 브로커 주소

파일: `config/config.go`

```go
KafkaBrokers = []string{"Kafka00Service:9092", "Kafka01Service:9092", "Kafka02Service:9092"}
```

### 4.2 PostgreSQL DSN

파일: `config/config.go`

```go
Dsn = "postgres://capstone2:block1234@postgres:5432/user_info?sslmode=disable"

```

### 4.3 KMA(기상청) 일사량 API Key

프록시 노드는 일사량 기반의 서명 점수(기본 점수) 계산을 위해 KMA API를 호출 해야합니다.

- 기본 설정: `config.KMAAuthKey`
- 환경변수:
    - `KMA_AUTH_KEY`
    - `KMA_STN` (기본 `ALL`)
    - `KMA_TM` (조회 시간)
    - `KMA_BACKOFF_HOURS`
    - `KMA_OUT_PATH`

- 예시

```bash
export KMA_AUTH_KEY="YOUR_KEY"
export KMA_STN="ALL"

```

### 4.4 Full Node RPC 주소 (tx 조회)

파일: `consumer/tx_hash.go` 

```go
resp, err := QueryTxBriefViaRPC("YOUR_IP_ADDRESS", "26657", txHash)

```

---

## 5. Kafka 토픽

토픽 이름은 `config/config.go`에 정의되어 있습니다.

### 5.1 토픽 목록

| 목적 | Topic |
| --- | --- |
| (요청) 디바이스 → 주소 매핑 | `topic-address-mapping-req` |
| (응답) 디바이스 → 주소 매핑 | `topic-address-mapping-res` |
| (요청) 유권자 수 요청 | `topic-votemember-count-req` |
| (응답) 유권자 수 응답 | `topic-votemember-count-res` |
| (요청) 서명자 보상 요청 | `topic-votemember-reward-req` |
| (응답) 서명자 보상 결과 | `topic-votemember-reward-res` |
| (저장) tx hash 저장 이벤트 | `topic-txhash-save` |
| (요청) tx hash 조회 요청 | `topic-txhash-request` |
| (응답) tx hash 조회 응답 | `topic-txhash-response` |
| (요청) 블록 생성자 선택 요청 | `topic-blockcreator-request` |
| (응답) 블록 생성자 선택 결과 | `topic-blockcreator-response` |



### 5.2 토픽 생성 명령어(예시)

```bash
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-address-mapping-req --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-address-mapping-res --partitions 1 --replication-factor 1

kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-votemember-count-req --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-votemember-count-res --partitions 1 --replication-factor 1

kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-votemember-reward-req --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-votemember-reward-res --partitions 1 --replication-factor 1

kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-txhash-save --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-txhash-request --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-txhash-response --partitions 1 --replication-factor 1

kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-blockcreator-request --partitions 1 --replication-factor 1
kafka-topics.sh --bootstrap-server localhost:9092 --create --topic topic-blockcreator-response --partitions 1 --replication-factor 1

```

---

## 6. PostgreSQL

### 6.1 DB/계정 생성(예시)

```sql
CREATE USER capstone2 WITH PASSWORD 'block1234';
CREATE DATABASE user_info OWNER capstone2;

```

### 6.2 기본 테이블 생성(권장 스키마)

아래를 `schema.sql`로 저장 후 실행하세요.

```sql
-- 사용자/디바이스 정보
CREATE TABLE IF NOT EXISTS userData (
  node_id     TEXT NOT NULL,
  device_id   TEXT NOT NULL,
  password    TEXT NOT NULL,
  public_key  TEXT,
  created_at  TIMESTAMPTZ DEFAULT NOW(),
  address     TEXT,
  PRIMARY KEY (node_id, device_id)
);

-- tx hash 정보
CREATE TABLE IF NOT EXISTS solar_archive (
  address     TEXT NOT NULL,
  hash        TEXT NOT NULL UNIQUE,
  created_at  TIMESTAMPTZ DEFAULT NOW()
);

-- 서명자 점수 누적
CREATE TABLE IF NOT EXISTS vote_counter (
  address     TEXT PRIMARY KEY,
  last_time   TIMESTAMPTZ,
  count       DOUBLE PRECISION NOT NULL DEFAULT 0
);

```

실행:

```bash
psql "postgres://capstone2:block1234@127.0.0.1:5432/user_info?sslmode=disable" -f schema.sql

```

---

## 7. 실행 방법

### 7.1 로컬 실행

```bash
go mod download
go run .

```

또는 빌드 후 실행:

```bash
go build -o oracle-app .
./oracle-app

```

서버에서 아래가 로그가 출력될 경우 HTTP 서버가 열린 상태입니다.

```
Server running on :3001

```

### 7.2 Docker 실행

프로젝트에 `docker-compose.yaml`이 포함되어 있습니다.

```bash
docker compose up --build

```

## 8. Trouble Shooting

### 8.1 Kafka 연결이 실패 할 경우

- `config.KafkaBrokers`가 현재 실행 환경에서 접근 가능한 주소인지 확인
- 토픽이 존재하는지 확인

### 8.2 PostgreSQL 연결이 실패 할 경우

- `config.Dsn`의 host/port/user/password/dbname 확인
- `sslmode=disable` 여부 확인(로컬이면 보통 disable)

### 8.3 tx hash 조회가 실패 할 경우

- `consumer/tx_hash.go`의 Tendermint RPC 주소 하드코딩 값을 실제 풀노드 IP로 변경했는지 확인
- 풀노드에서 `26657` RPC가 열려 있는지 확인(Full Node config의 `laddr` 설정)
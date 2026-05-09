# Full Node Setup Guide

해당 README.md는 **풀 노드 실행** 및 **Genesis 블록 생성 이후 필수 초기 REC 담보 예치(Deposit Collateral)** 설정 방법을 안내합니다.

- **대상 디렉토리:** Mobile_BolckChain_Network/cosmos
> 
> 
- **결과물:** `build/simd` 바이너리 생성 및 `private/.simapp` 기반 풀 노드 구동
> 

---

## Table of Contents

- [1. Setup](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [2. Required Ports](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [3. Config](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [3.1 config.toml (laddr 바인딩)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [3.2 app.toml (REST API 활성화)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [4. Run Full Node](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [5. Deposit Initial REC Collateral (Required)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [5.1 Option A: One-liner JSON](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
    - [5.2 Option B: File-based rec-meta](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [6. Query APIs (REST)](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)
- [Troubleshooting](https://www.notion.so/1ef9ee2fb5c780858cb6f39c97790776?pvs=21)

---

## 1. Setup

`cosmos` 디렉토리에서 setup 스크립트를 실행합니다.

```bash
./setup.sh
```

빌드가 완료되면 아래 경로에 해당 바이너리가 생성됩니다.

```
build/simd

```

## 2. Required Ports

풀노드 실행 및 외부 연동에 필요한 포트입니다.

| Port | Component | Purpose | Config Location |
| --- | --- | --- | --- |
| 26656 | Tendermint P2P | 노드 간 P2P 네트워크 통신 | `private/.simapp/config/config.toml` |
| 26657 | Tendermint RPC | RPC(상태 조회/트랜잭션 브로드캐스트 등) | `private/.simapp/config/config.toml` |
| 1317 | REST API | REST Query API (`/cosmos/...`) | `private/.simapp/config/app.toml` |
| 9090 | gRPC (선택) | gRPC Query (환경에 따라 사용) | `private/.simapp/config/app.toml` |

> 외부에서 접근 가능하도록 하려면, 아래 Config 섹션에서 바인딩 IP를 0.0.0.0 으로 설정해야합니다.
> 
> 
> Docker 환경이라면 포트 매핑도 필요합니다 (예: `-p 1317:1317 -p 26657:26657 -p 26656:26656`).
> 

---

## 3. Config

풀노드는 기본적으로 `private/.simapp` 홈 디렉토리를 사용합니다.

아래 설정을 통해 **외부 접근** 및 **REST API 사용**이 가능합니다.

### 3.1 config.toml (laddr 바인딩)

**파일 경로**

```
private/.simapp/config/config.toml

```

`laddr`가 포함된 모든 항목의 IP를 `0.0.0.0`으로 설정합니다.

(예: RPC, P2P 등)

**예시**

```toml
laddr = "tcp://0.0.0.0:26657"

```

---

### 3.2 app.toml (REST API 활성화)

**파일 경로**

```
private/.simapp/config/app.toml

```

아래와 같이 `[api]` 섹션을 설정합니다.

```toml
[api]
enable = true
swagger = true
address = "tcp://0.0.0.0:1317"

```

- `enable = true` : REST API 서버 활성화
- `swagger = true` : Swagger UI 활성화(가능한 경우)
- `address` : API 바인딩 주소/포트

---

## 4. Run Full Node

노드를 실행합니다.

```bash
./build/simd start \ --home ./private/.simapp

```

정상 실행 성공 시 아래 컴포넌트가 함께 구동합니다.

- Tendermint(Consensus)
- Cosmos SDK App
- REST API 서버 (`1317`)

---

## 5. Deposit Initial REC Collateral (Required)

> **※ Genesis 이후 반드시 1회 이상 초기 담보 예치를 수행해야 보상/REC 관련 시스템이 정상 동작합니다.**
> 

아래 트랜잭션을 실행하여 REC 담보를 예치합니다.

### 5.1 Option A: One-liner JSON

```bash
./build/simd tx reward deposit-collateral \
  --rec-meta '{
    "facility_id": "FAC12345",
    "facility_name": "Solar Farm B",
    "location": "Seoul",
    "technology_type": "solar",
    "capacity_mw": "10",
    "registration_date": "2025-11-02",
    "certified_id": "CERT19969B",
    "issue_date": "2025-01-02",
    "generation_start_date": "2025-01-01",
    "generation_end_date": "2025-12-31",
    "measured_volume_mwh": "5000",
    "retired_date": "",
    "retirement_purpose": "",
    "status": "active",
    "timestamp": "2025-09-20T01:31:50Z"
  }' \
  --from alice \
  --chain-id learning-chain-1 \
  --home ./private/.simapp \
  --keyring-backend test \
  --gas auto \
  --gas-adjustment 1.2 \
  --yes

```

---

### 5.2 Option B: File-based rec-meta

장문의 JSON을 커맨드를 통해 직접 입력하지 않고, `rec-meta.json` 파일로 분리해서 관리할 수 있습니다.

### 1) JSON 파일 생성

```bash
cat > rec-meta.json <<'JSON'
{
  "facility_id": "FAC12345",
  "facility_name": "Solar Farm B",
  "location": "Seoul",
  "technology_type": "solar",
  "capacity_mw": "10",
  "registration_date": "2025-11-02",
  "certified_id": "CERT19969B",
  "issue_date": "2025-01-02",
  "generation_start_date": "2025-01-01",
  "generation_end_date": "2025-12-31",
  "measured_volume_mwh": "5000",
  "retired_date": "",
  "retirement_purpose": "",
  "status": "active",
  "timestamp": "2025-09-20T01:31:50Z"
}
JSON

```

### 2) 파일을 읽어 `-rec-meta`로 전달

> 별도의 --rec-meta-file 옵션이 없이, Shell을 통한 **파일 기반 입력**을 구현할 수 있습니다.
> 

```bash
./build/simd tx reward deposit-collateral \
  --rec-meta "$(cat ./rec-meta.json)" \
  --from alice \
  --chain-id learning-chain-1 \
  --home ./private/.simapp \
  --keyring-backend test \
  --gas auto \
  --gas-adjustment 1.2 \
  --yes

```

---

### Notes

- `-from alice`: 로컬 키링에 `alice` 계정이 존재해야 합니다.
- `-keyring-backend test`: 개발 편의용 키링 백엔드입니다.
- `-gas auto`: 가스 자동 산정

키 존재 여부 확인:

```bash
./build/simd keys list \
  --home ./private/.simapp \
  --keyring-backend test

```

---

## 6. Query APIs (REST)

REST API 서버는 기본적으로 `1317` 포트에서 실행됩니다.

Base URL:

```
http://localhost:1317

```

### 6.1 예치된 REC 목록 조회

```bash
curl http://localhost:1317/cosmos/reward/v1beta1/rec_list

```

### 6.2 총 코인 발행량 조회

```bash
curl http://localhost:1317/cosmos/reward/v1beta1/supply

```

### 6.3 발급된 REC 블록(트랜잭션 노드) 조회

```bash
curl http://localhost:1317/cosmos/reward/v1beta1/tx_node_list

```

---

## Troubleshooting

### API 서버 접속이 안 되는 경우

- `app.toml`의 `[api] enable = true` 여부 확인
- `address = "tcp://0.0.0.0:1317"` 로 바인딩했는지 확인
- 로컬 방화벽/포트 점유 확인

### RPC/네트워크 접근이 안 되는 경우

- `config.toml`의 모든 `laddr`가 `0.0.0.0`으로 변경되었는지 확인
- Docker 환경이라면 포트 매핑 확인 (`1317`, `26657`, `26656`)

### deposit-collateral 실패 시

- `alice` 키 존재 여부 확인 (`keys list`)
- `chain-id` 일치 여부 확인: `learning-chain-1`
- 노드가 실행 중인지 확인 (`simd start`), 그리고 동일한 `-home` 사용 여부 확인
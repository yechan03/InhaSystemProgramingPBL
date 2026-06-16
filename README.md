# System Programming PBL

인하대학교 시스템프로그래밍 PBL 프로젝트.
개인 계정 관리 + 미니게임 종합 로비를 C 표준 라이브러리와 sh 스크립트로 구현.

## Project Structure

```
InhaSystemProgramingPBL/
├── Makefile                # 빌드 정의 (gcc + C99)
├── README.md
├── .gitignore
│
├── src/                    # C 소스 코드
│   ├── account.h           # 계정 모듈 인터페이스
│   ├── account.c           # 회원가입 / 로그인 / 해시
│   └── lobby.c             # 메인 메뉴, 로비 진입점 (main)
│
├── scripts/                # 빌드·실행 sh 스크립트
│   ├── init.sh             # data/ 디렉토리·빈 파일 생성
│   ├── build.sh            # gcc 컴파일 → bin/lobby
│   ├── run.sh              # 빌드 후 실행
│   └── clean.sh            # bin/ 정리 (data/ 보존)
│
├── data/                   # 런타임 데이터 (git 추적 제외)
│   └── accounts.txt        # username:hash 형식의 계정 저장소
│
└── bin/                    # 빌드 산출물 (git 추적 제외)
    └── lobby               # 컴파일된 실행파일
```

## 디렉토리 역할

| 디렉토리   | 역할                                       |
| ---------- | ------------------------------------------ |
| `src/`     | C 소스 / 헤더. 모든 구현 코드가 위치       |
| `scripts/` | sh 스크립트 (빌드·실행·초기화·정리)        |
| `data/`    | 사용자 데이터 (계정 등). 로컬 전용         |
| `bin/`     | gcc 컴파일 산출물. `make clean` 시 삭제됨  |

## 사용 라이브러리

C 표준 라이브러리만 사용:

- `<stdio.h>`  — 입출력 (`fopen`, `fprintf`, `fgets` 등)
- `<stdlib.h>` — `system()`, `strtoul`, `atoi`
- `<string.h>` — 문자열 처리

비밀번호 에코 차단은 `system("stty -echo")` 호출로 처리

## 빌드 / 실행

```sh
chmod +x scripts/*.sh         # 최초 1회
./scripts/run.sh              # 빌드 + 실행

# 또는
make run
```
      
## 동작 추적 예시 (`yechan` / `1234`)
```
회원가입 시 `hash_credential("yechan", "1234")` 호출의 단계별 흐름:
```
초기값:    h = 5381

"yechan" 한 글자씩:
'y' 121 → h = (h * 33) ^ 121
'e' 101 → h = (h * 33) ^ 101
'c'  99 → h = (h * 33) ^  99
'h' 104 → h = (h * 33) ^ 104
'a'  97 → h = (h * 33) ^  97
'n' 110 → h = (h * 33) ^ 110

구분자 ':' →  h = (h * 33) ^ 58      // 아이디/비번 경계 마커

"1234" 한 글자씩:
'1'  49 → h = (h * 33) ^ 49
'2'  50 → h = (h * 33) ^ 50
'3'  51 → h = (h * 33) ^ 51
'4'  52 → h = (h * 33) ^ 52

최종 h = 13795222493806861027   (unsigned long, 64bit)
```

> `(h << 5) + h == h * 33` — 곱셈 대신 시프트+덧셈으로 빠르게 계산하는 djb2 의 관용구. 본 프로젝트는
원본 djb2 의 `+` 대신 `^`(XOR) 을 사용한 변종.

저장 결과 (`data/accounts.txt`):
  ```
yechan:13795222493806861027
```

로그인 시 `hash_credential("yechan", 입력된_비번)` 을 다시 계산해 저장된 정수와 단순 비교만 수행합니다
원본 비밀번호는 디스크 어디에도 남지 않습니다.


## 데이터 파일 예시

`data/accounts.txt`:
```
username:hashvalue
```
한 줄에 한 계정. 비밀번호는 djb2 변형 해시 + 아이디 솔트로 저장.


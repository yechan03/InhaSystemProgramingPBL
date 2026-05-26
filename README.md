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
│   ├── score.h             # 점수 및 랭킹 모듈 인터페이스
│   ├── score.c             # 최고점수 비교·갱신 / 시간 연산 / 순위표 출력
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
│   └── scores.txt          # game_num:username:score:time 형식의 점수 저장소
│
└── bin/                    # 빌드 산출물 (git 추적 제외)
│   └── lobby               # 컴파일된 실행파일
│
└── games/                  # [추가] 독립 실행형 미니게임 바이너리 저장소
    └── game1               # 컴파일된 1번 미니게임 실행파일
```

## 디렉토리 역할

| 디렉토리   | 역할                                       |
| ---------- | ------------------------------------------ |
| `src/`     | C 소스 / 헤더. 모든 구현 코드가 위치       |
| `scripts/` | sh 스크립트 (빌드·실행·초기화·정리)        |
| `data/`    | 사용자 데이터 (계정 등). 로컬 전용         |
| `games/`	 | 독립 프로세스로 구동될 게임 바이너리 모음    |
| `bin/`     | gcc 컴파일 산출물. `make clean` 시 삭제됨  |

## 사용 라이브러리

C 표준 라이브러리만 사용:

- `<stdio.h>`  — 입출력 (`fopen`, `fprintf`, `fgets` 등)
- `<stdlib.h>` — `system()`, `strtoul`, `atoi`
- `<string.h>` — 문자열 처리
- `<time.h>`   — 시스템 시간 연산 및 포맷팅 (time, localtime, strftime)

비밀번호 에코 차단은 `system("stty -echo")` 호출로 처리 (POSIX 헤더 미사용).

## 빌드 / 실행

```sh
chmod +x scripts/*.sh         # 최초 1회
./scripts/run.sh              # 빌드 + 실행

# 또는
make run
```

## 데이터 파일 형식

`data/accounts.txt`:
```
username:hashvalue
```
한 줄에 한 계정. 비밀번호는 djb2 변형 해시 + 아이디 솔트로 저장.

`data/scores.txt`:
```
game_number:username:high_score:timestamp
```
한 줄에 하나의 최고 점수 레코드를 보관. 
게임이 끝날 때마다 기존 최고 점수와 실시간 비교 연산을 수행하여, 더 높은 점수를 달성했을 때만 현재 리눅스 시스템 시간과 함께 데이터를 동적 갱신.

## 비밀번호 저장 방식 (해싱)

이 프로젝트는 비밀번호를 **암호화가 아니라 해싱(hashing)** 으로 저장합니다.

| 구분      | 암호화 (Encryption)        | 해싱 (이 프로젝트 방식)        |
| --------- | -------------------------- | ------------------------------ |
| 방향성    | 양방향 (복호화 가능)       | 단방향 (되돌릴 수 없음)        |
| 키        | 필요                       | 없음                           |
| 용도      | 통신/저장 비밀 유지        | 동일성 검증 (= 비밀번호 검사)  |

비밀번호는 원본을 복구할 필요 없이 "맞는지만 확인"하면 되기 때문에 해싱이 정답입니다. 평문이나 가역 암호화로 저장하면 서버가 탈취당했을 때 비밀번호가 그대로 노출됩니다.

### 해시 알고리즘 (`src/account.c` 의 `hash_credential`)

djb2 의 XOR 변종을 사용하며, 아이디를 솔트(salt)로 함께 섞습니다.

```
초기값 h = 5381
아이디 한 글자씩:   h = (h * 33) ^ char        // (h << 5) + h == h * 33
구분자 ':':          h = (h * 33) ^ ':'         // 아이디/비번 경계 마커
비밀번호 한 글자씩: h = (h * 33) ^ char
최종 h (unsigned long, 64bit) 를 10진수 문자열로 저장
```

### 동작 추적 예시 (`yechan` / `1234`)

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

> `(h << 5) + h == h * 33` — 곱셈 대신 시프트+덧셈으로 빠르게 계산하는 djb2 의 관용구. 본 프로젝트는 원본 djb2 의 `+` 대신 `^`(XOR) 을 사용한 변종.

저장 결과 (`data/accounts.txt`):

```
yechan:13795222493806861027
```

로그인 시 `hash_credential("yechan", 입력된_비번)` 을 다시 계산해 저장된 정수와 단순 비교만 수행합니다. 원본 비밀번호는 디스크 어디에도 남지 않습니다.

### 멀티 프로세스 기반 게임 구동 및 예외 제어
본 프로젝트는 대규모 아케이드 플랫폼의 구조를 모방하여, 메인 로비 프레임워크와 미니게임을 독립된 개별 프로세스로 격리하여 구동합니다.

1. 프로세스 분기 및 대체 (`fork & execl`)
유저가 로비에서 미니게임을 선택하면, 로비는 `fork()`를 통해 자식 프로세스를 생성합니다.

자식 프로세스는 `games/game1`과 같은 독립 실행형 바이너리 경로를 찾아 `execl()`을 호출함으로써, 자신의 메모리 공간을 해당 미니게임 프로그램으로 완전 대체합니다.

이때, 로그인된 사용자의 ID(`username`)를 프로그램 인자(`argv[1]`)로 안전하게 넘겨주어 게임 내 무결성을 유지합니다.

2. 파이프(`pipe`)를 활용한 실행 파일 누락 예외 처리
`execl()`이 실패할 경우(게임 바이너리가 컴파일되지 않았거나 누락된 경우)를 대비하여, `fork()` 직전 익명 파이프(`pipe`)를 개설합니다.

자식이 `execl()`에 실패하면 파이프에 에러 시그널을 쓰고 즉시 종료됩니다.

부모 프로세스는 이 파이프 신호를 감지하여 게임 파일 누락 에러를 정확하게 판정합니다. 이로 인해 유저가 실제 미니게임에서 정직하게 점수를 획득하고 정상 종료했을 때 에러로 오인하는 충돌 버그를 완벽히 차단합니다.

3. 종료 코드 기반 점수 회수 (`wait & WEXITSTATUS`)
미니게임이 정상 종료되면서 `exit(final_score);`를 호출하면, 부모 프로세스는 커널 단계에서 `wait(&status)`로 대기하다가 `WEXITSTATUS(status)`를 통해 소멸한 자식이 남긴 점수를 안전하게 가로챕니다.

### 최고 점수 및 순위표(Leaderboard) 출력 포맷
로비 메뉴에서 랭킹 조회를 요청할 경우, C 표준 printf 서식 지정자를 활용하여 터미널 환경에 가독성 높은 격자 대시보드를 출력합니다.
```
=======================================================
               INHA ARCADE LEADERBOARD                 
=======================================================
 GAME |   PLAYER ID    |  HIGH SCORE  |     DATE TIME    
-------------------------------------------------------
  #1  | GD_Rowl        |   95         | 2026-05-24 14:36:12
  #1  | yechan         |   80         | 2026-05-24 15:02:45
=======================================================
```


# System Programming PBL

System Programming PBL project from Inha University.  
A personal account management + mini-game integrated lobby implemented using only the C standard library and `sh` scripts.

## Project Structure

```text
InhaSystemProgramingPBL/
├── Makefile                # Build configuration (gcc + C99)
├── README.md
├── .gitignore
│
├── src/                    # C source code
│   ├── account.h           # Account module interface
│   ├── account.c           # Sign-up / login / hashing
│   ├── score.h             # Score & ranking module interface
│   ├── score.c             # High-score comparison & update / time operations / leaderboard output
│   └── lobby.c             # Main menu and lobby entry point (main)
│
├── scripts/                # Build & execution shell scripts
│   ├── init.sh             # Creates data/ directory and empty files
│   ├── build.sh            # gcc compilation → bin/lobby
│   ├── run.sh              # Build and execute
│   └── clean.sh            # Cleans bin/ (preserves data/)
│
├── data/                   # Runtime data (excluded from git tracking)
│   ├── accounts.txt        # Account storage in username:hash format
│   └── scores.txt          # Score storage in game_num:username:score:time format
│
├── bin/                    # Build artifacts (excluded from git tracking)
│   └── lobby               # Compiled executable
│
└── games/                  # [Additional] Standalone mini-game binary repository
    └── game1               # Compiled executable for mini-game #1
```

## Directory Roles

| Directory   | Purpose |
| ---------- | ------------------------------------------ |
| `src/`     | C source/header files. Contains all implementation code |
| `scripts/` | Shell scripts (build, run, initialize, clean) |
| `data/`    | User data storage (accounts, etc.). Local-only |
| `games/`   | Collection of game binaries executed as independent processes |
| `bin/`     | gcc build artifacts. Removed by `make clean` |

## Libraries Used

Only the C standard library is used:

- `<stdio.h>`  — File I/O (`fopen`, `fprintf`, `fgets`, etc.)
- `<stdlib.h>` — `system()`, `strtoul`, `atoi`
- `<string.h>` — String processing
- `<time.h>`   — System time operations and formatting (`time`, `localtime`, `strftime`)

Password echo suppression is implemented using `system("stty -echo")` without POSIX-specific headers.

## Build / Run

```sh
chmod +x scripts/*.sh         # First-time setup
./scripts/run.sh              # Build + run

# or
make run
```

## Data File Formats

`data/accounts.txt`:
```text
username:hashvalue
```

One account per line. Passwords are stored using a modified djb2 hash combined with username-based salting.

`data/scores.txt`:
```text
game_number:username:high_score:timestamp
```

One high-score record per line.  
Whenever a game ends, the current score is compared against the existing best score in real time. The record is dynamically updated only if a higher score is achieved, together with the current Linux system timestamp.

## Password Storage Method (Hashing)

This project stores passwords using **hashing**, not encryption.

| Category | Encryption | Hashing (Used in This Project) |
| --------- | -------------------------- | ------------------------------ |
| Direction | Two-way (decryptable)      | One-way (irreversible)         |
| Key       | Required                   | None                           |
| Purpose   | Keeping secrets in storage/communication | Identity verification (= password validation) |

Since passwords only need to be verified — not restored — hashing is the correct approach. Storing plaintext or reversibly encrypted passwords would expose all credentials if the server were compromised.

### Hash Algorithm (`hash_credential` in `src/account.c`)

A XOR-variant of djb2 is used, with the username mixed in as a salt.

```text
Initial value h = 5381

For each character in username:
    h = (h * 33) ^ char      // (h << 5) + h == h * 33

Separator ':':
    h = (h * 33) ^ ':'

For each character in password:
    h = (h * 33) ^ char

Final h (unsigned long, 64-bit)
→ stored as a decimal string
```

### Execution Trace Example (`yechan` / `1234`)

Step-by-step flow of:

```c
hash_credential("yechan", "1234")
```

during account registration:

```text
Initial value:
    h = 5381

Processing "yechan":
  'y' 121 → h = (h * 33) ^ 121
  'e' 101 → h = (h * 33) ^ 101
  'c'  99 → h = (h * 33) ^  99
  'h' 104 → h = (h * 33) ^ 104
  'a'  97 → h = (h * 33) ^  97
  'n' 110 → h = (h * 33) ^ 110

Separator ':':
    h = (h * 33) ^ 58

Processing "1234":
  '1'  49 → h = (h * 33) ^ 49
  '2'  50 → h = (h * 33) ^ 50
  '3'  51 → h = (h * 33) ^ 51
  '4'  52 → h = (h * 33) ^ 52

Final result:
    h = 13795222493806861027
    (unsigned long, 64-bit)
```

> `(h << 5) + h == h * 33` — the classic djb2 optimization using bit-shifting and addition instead of multiplication. This project uses a XOR (`^`) variant instead of the original `+` version.

Stored result (`data/accounts.txt`):

```text
yechan:13795222493806861027
```

During login, the system recomputes:

```c
hash_credential("yechan", input_password)
```

and simply compares the resulting integer with the stored value. The original plaintext password is never stored anywhere on disk.

### Multi-Process-Based Game Execution & Exception Handling

This project mimics the architecture of large-scale arcade platforms by isolating the main lobby framework and each mini-game into independent processes.

#### 1. Process Forking & Replacement (`fork` & `execl`)

When a user selects a mini-game from the lobby, the lobby creates a child process using `fork()`.

The child process locates an independent executable such as `games/game1` and calls `execl()` to completely replace its own memory space with the mini-game program.

The currently authenticated user's ID (`username`) is securely passed as a program argument (`argv[1]`) to preserve in-game integrity.

#### 2. Missing Executable Exception Handling Using `pipe`

To handle situations where `execl()` fails (for example, when the game binary is missing or not compiled), an anonymous pipe (`pipe`) is created immediately before `fork()`.

If `execl()` fails, the child process writes an error signal into the pipe and exits immediately.

The parent process monitors this pipe signal to accurately detect missing game files. This completely prevents collision bugs where a legitimate game termination could mistakenly be interpreted as an execution failure.

#### 3. Score Retrieval via Exit Codes (`wait` & `WEXITSTATUS`)

When the mini-game terminates normally using:

```c
exit(final_score);
```

the parent process waits at the kernel level using:

```c
wait(&status);
```

and safely retrieves the score left by the terminated child process through:

```c
WEXITSTATUS(status)
```

### High Score & Leaderboard Output Format

When the user requests ranking information from the lobby menu, the program uses C standard `printf` format specifiers to render a readable terminal dashboard:

```text
=======================================================
               INHA ARCADE LEADERBOARD
=======================================================
 GAME |   PLAYER ID    |  HIGH SCORE  |     DATE TIME
-------------------------------------------------------
  #1  | GD_Rowl        |   95         | 2026-05-24 14:36:12
  #1  | yechan         |   80         | 2026-05-24 15:02:45
=======================================================
```

# System Programming PBL

인하대학교 시스템프로그래밍 PBL 프로젝트.
개인 계정 관리 + 미니게임 종합 로비를 C 표준 라이브러리와 sh 스크립트로 구현.

## Project Structure

```
InhaSystemProgramingPBL/
├── Makefile                 # 빌드 정의 (gcc + C99)
├── README.md
├── .gitignore
│
├── src/                     # C 소스 코드
│   ├── account.h            # 계정 모듈 인터페이스
│   ├── account.c            # 회원가입 / 로그인 / 해시 / GitHub 계정 검증
│   ├── score.h              # 점수 및 랭킹 모듈 인터페이스
│   ├── score.c              # 최고점수 비교·갱신 / 시간 연산 / 순위표 출력
│   ├── game1.c              # 1번 미니게임 VI-RPG 소스 (bash 원작을 C로 포팅)
│   └── lobby.c              # 메인 메뉴, 로비 진입점 (main)
│
├── scripts/                 # 빌드·실행·연동 sh 스크립트
│   ├── init.sh              # data/ 디렉토리·빈 파일 생성
│   ├── build.sh             # gcc 컴파일 → bin/lobby + games/game1·2·3·5
│   ├── run.sh               # 빌드 후 실행
│   ├── clean.sh             # bin/, games/ 바이너리 정리 (data/ 보존)
│   ├── github_check.sh      # GitHub username 존재 확인 (회원가입 시 system 호출)
│   ├── github_stats.sh      # GitHub PushEvent 수집 (game2 가 popen 호출)
│   ├── play_sound_tetris.sh # 테트리스 효과음 비동기 재생 (game3 가 system 호출)
│   └── sys_monitor.sh       # 호스트 CPU 사용량 파싱 (game3 가 popen 호출)
│
├── data/                    # 런타임 데이터 (git 추적 제외)
│   ├── accounts.txt         # id:hash:github 형식의 계정 저장소
│   └── scores.txt           # game_num:username:score:time 형식의 점수 저장소
│
├── bin/                     # 빌드 산출물 (git 추적 제외)
│   └── lobby                # 컴파일된 로비 실행파일
│
├── games/                   # 독립 프로세스로 구동되는 미니게임 모음
│   ├── game1                # 1번 VI-RPG (src/game1.c 빌드 산출물)
│   ├── game2.c / game2      # 2번 GitHub 다마고치 (소스 + 빌드 산출물)
│   ├── game3.c / game3      # 3번 VI-TETRIS (소스 + 빌드 산출물)
│   ├── game4.sh             # 4번 자원 경영 시뮬레이션 진입 스크립트 (bash)
│   ├── management_game/     # game4.sh 가 source 하는 모듈 모음
│   │   ├── change_turn.sh / input.sh / display.sh / resources.sh
│   │   └── utils.sh / contracts_management.sh / market.sh / events.sh
│   ├── game5.c / game5      # 5번 Knight's Tour 기사의 여행 (소스 + 빌드 산출물)
│   └── game_bash.sh         # VI-RPG bash 원작 (보존용)
│
└── sound/                   # 하드웨어 오디오 출력을 위한 WAV 리소스 저장소
    ├── game3_lock.wav       # 블록 고정 효과음
    ├── game3_clear.wav      # 라인 클리어 효과음
    ├── game3_PerfectClear.wav # 퍼펙트 클리어 업적 효과음
    └── game3_gameover.wav   # 게임 오버 효과음
```

## 디렉토리 역할

| 디렉토리   | 역할                                       |
| ---------- | ------------------------------------------ |
| `src/`     | C 소스 / 헤더. 모든 구현 코드가 위치       |
| `scripts/` | sh 스크립트 (빌드·실행·초기화·정리)        |
| `data/`    | 사용자 데이터 (계정 등). 로컬 전용         |
| `games/`	 | 독립 프로세스로 구동될 게임 모음 (바이너리 + game4.sh 스크립트 + 모듈)    |
| `bin/`     | gcc 컴파일 산출물. `make clean` 시 삭제됨  |
| `sound/`   | 게임에서 사용할 오디오 서브시스템 리소스 저장소  |

## 프로그램 흐름도 (Flow Chart)

로비 진입부터 회원가입 / 로그인 / 미니게임(fork·exec) / 점수 회수까지의 전체 제어 흐름.
회색으로 표시된 노드는 sh 스크립트(`system()` / `popen()`)로 외부 도구(curl)를 호출하는 지점이다.

```mermaid
flowchart TD
    Start([프로그램 시작]) --> Menu{"메인 메뉴<br/>1 가입 / 2 로그인 / 0 종료"}

    %% ---- 회원가입 ----
    Menu -->|1 회원가입| Reg["ID · GitHub username · PW 입력"]
    Reg --> GHChk["github_check.sh (system)<br/>GitHub username 존재 확인"]
    GHChk -->|존재 200| SaveAcc[("accounts.txt 저장<br/>id:hash:github")]
    GHChk -->|없음 404 / 오류| Menu
    SaveAcc --> Menu

    %% ---- 로그인 ----
    Menu -->|2 로그인| Login["ID · PW 입력"]
    Login --> Verify{"hash_credential 일치?"}
    Verify -->|불일치| Menu
    Verify -->|일치| Lobby{"로비 메뉴<br/>1~5 게임 / 9 순위표 / 0 로그아웃"}

    Menu -->|0 종료| End([프로그램 종료])

    %% ---- 게임 실행 공통 ----
    Lobby -->|1~5 선택| Fork[["fork() + pipe()<br/>자식 프로세스 생성<br/>(로비는 SIGINT 잠시 무시)"]]
    Fork --> Exec["execl(games/gameN, ...)<br/>argv[1]=ID, argv[2]=github, argv[3]=파이프 fd"]
    Exec -->|execl 실패| ExecFail["파이프에 -1 write<br/>바이너리 누락 안내"]
    ExecFail --> Lobby

    %% ---- Game 1 분기 (VI-RPG) ----
    Exec --> G1["game1 : VI-RPG<br/>(bash 원작의 C 포팅)"]
    G1 -->|게임 종료| ExitScore1["exit(score) 종료코드 반환"]

    %% ---- Game 2 분기 ----
    Exec --> G2["game2 : GitHub Tamagotchi"]
    G2 --> Stats["github_stats.sh (popen)<br/>마지막 push 일수 · PushEvent 개수"]
    Stats --> Face["표정 렌더링 (제자리 갱신)<br/>r / q 입력 대기"]
    Face -->|r 새로고침| Stats
    Face -->|q 종료| ExitScore["exit(score) 종료코드 반환"]

    %% ---- Game 3 분기 (테트리스 시스템 연동) ----
    Exec --> G3["game3 : VI-TETRIS"]
    G3 --> SysMon["sys_monitor.sh (popen)<br/>호스트 커널 CPU 사용량 실시간 수집"]
    SysMon --> SpeedMod["CPU 부하 연동형 타이머 변조<br/>drop_interval 실시간 가속"]
    SpeedMod --> GameLoop{"인게임 루프<br/>키 입력 / 타이머 하강"}

    GameLoop -->|블록 고정 / 라인 제거 / 올클리어| SndScript["play_sound_tetris.sh (system)<br/>aplay 백그라운드 & 비동기 효과음 재생"]
    SndScript --> GameLoop

    GameLoop -->|q 입력 / Game Over| PipeScore["argv[3] 파이프에 score write<br/>(8bit 제한 없는 대형 점수)"]

    %% ---- Game 4 분기 (자원 경영 시뮬레이션) ----
    Exec --> G4["game4.sh : Factory-ism (bash)<br/>management_game/ 모듈 source"]
    G4 -->|Ctrl+C (SIGINT trap) / 메뉴 종료| PipeScore4["report_score() : 자산 환산 점수 계산<br/>(money+coal·2+ore·3+iron·5+steel·10)/100<br/>255 clamp 후 argv[3] 파이프에 4바이트 정수 write"]
    PipeScore4 --> Wait

    %% ---- Game 5 분기 (기사의 여행) ----
    Exec --> G5["game5 : Knight's Tour<br/>기사의 여행 (실시간 키 입력)"]
    G5 -->|게임 종료| PipeScore5["argv[3] 파이프에 score write"]

    %% ---- 자식 종료 및 결산 공통 ----
    ExitScore1 --> Wait["부모 점수 회수 프로토콜:<br/>① 파이프 값 -1 → 실행 실패<br/>② 파이프 값 ≥0 → 그대로 점수 (game3·4·5)<br/>③ 파이프 비어있음 → wait() + WEXITSTATUS (game1·2)"]
    ExitScore --> Wait
    PipeScore --> Wait
    PipeScore5 --> Wait
    Wait --> SaveScore[("scores.txt<br/>최고점수 비교·갱신")]
    SaveScore --> Lobby

    %% ---- 순위표 / 로그아웃 ----
    Lobby -->|9 순위표| Board["show_leaderboard()<br/>랭킹 대시보드 출력"]
    Board --> Lobby
    Lobby -->|0 로그아웃| Menu

    %% ---- 외부 스크립트 노드 강조 ----
    classDef sh fill:#e8e8e8,stroke:#888,color:#000;
    class GHChk,Stats,SysMon,SndScript sh;
```

## 사용 라이브러리

C 표준 라이브러리만 사용:

- `<stdio.h>`  — 입출력 (`fopen`, `fprintf`, `fgets` 등)
- `<stdlib.h>` — `system()`, `strtoul`, `atoi`
- `<string.h>` — 문자열 처리
- `<time.h>`   — 시스템 시간 연산 및 포맷팅 (time, localtime, strftime)
- `<unistd.h>` — POSIX OS API 인터페이스. 파일 디스크립터 제어 및 프로세스 이미지 대체 (`read`, `close`, `execl`, `STDIN_FILENO`)
- `<sys/select.h>` — I/O 멀티플렉싱 커널 시스템 콜. 마이크로초 단위의 비동기 키 입력 감지 타이머 타임아웃 처리 (`select`)
- `<sys/wait.h>` — 자식 프로세스 생명 주기 관리 및 커널 시그널 추적 인터페이스 (`wait`)
- `<termios.h>` — 터미널 I/O 특성 변경 인터페이스. 버퍼 없는 로우 모드(Raw Mode) 진입 및 키 에코 차단 (`tcgetattr`, `tcsetattr`)
- `<signal.h>` — ISO C 표준 시그널 처리. 게임 실행 중 로비가 Ctrl+C(SIGINT)에 죽지 않도록 보호 (`signal`, `SIG_IGN`)

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
id:hashvalue:github_username
```
한 줄에 한 계정. 비밀번호는 djb2 변형 해시 + 아이디 솔트로 저장.
github 필드는 회원가입 시 `scripts/github_check.sh` 로 실존 여부를 검증한 GitHub username (game2 가 사용).
구버전 2필드(`id:hash`) 계정은 로그인 시 id 를 github 으로 폴백.

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

저장 결과 (`data/accounts.txt`, github 필드 포함 3필드):

```
yechan:13795222493806861027:yechan03
```

로그인 시 `hash_credential("yechan", 입력된_비번)` 을 다시 계산해 저장된 정수와 단순 비교만 수행합니다. 원본 비밀번호는 디스크 어디에도 남지 않습니다.

### 멀티 프로세스 기반 게임 구동 및 예외 제어
본 프로젝트는 대규모 아케이드 플랫폼의 구조를 모방하여, 메인 로비 프레임워크와 미니게임을 독립된 개별 프로세스로 격리하여 구동합니다.

1. 프로세스 분기 및 대체 (`fork & execl`)
유저가 로비에서 미니게임을 선택하면, 로비는 `fork()`를 통해 자식 프로세스를 생성합니다.

자식 프로세스는 `games/game1`과 같은 독립 실행형 바이너리 경로를 찾아 `execl()`을 호출함으로써, 자신의 메모리 공간을 해당 미니게임 프로그램으로 완전 대체합니다.

이때, 로그인된 사용자의 ID(`argv[1]`), GitHub username(`argv[2]`), 점수 전달용 파이프 fd(`argv[3]`)를 프로그램 인자로 안전하게 넘겨줍니다. game4 는 컴파일 바이너리가 아닌 bash 스크립트(`games/game4.sh`)지만, 커널이 `#!/bin/bash` 셔뱅을 해석하므로 동일한 `execl()` 경로로 실행됩니다.

2. 파이프(`pipe`)를 활용한 실행 파일 누락 예외 처리
`execl()`이 실패할 경우(게임 바이너리가 컴파일되지 않았거나 누락된 경우)를 대비하여, `fork()` 직전 익명 파이프(`pipe`)를 개설합니다.

자식이 `execl()`에 실패하면 파이프에 에러 신호(`-1`)를 쓰고 즉시 종료됩니다. 점수는 항상 0 이상이므로 -1 은 실행 실패 전용 신호로 안전합니다.

부모 프로세스는 이 파이프 신호를 감지하여 게임 파일 누락 에러를 정확하게 판정합니다. 이로 인해 유저가 실제 미니게임에서 정직하게 점수를 획득하고 정상 종료했을 때 에러로 오인하는 충돌 버그를 완벽히 차단합니다.

3. 하이브리드 점수 회수 프로토콜 (파이프 우선, 종료 코드 폴백)
부모 프로세스는 `wait(&status)`로 자식 종료를 대기한 뒤, 다음 순서로 점수를 회수합니다.

| 파이프 수신 값 | 해석 | 처리 |
| --- | --- | --- |
| `-1` | `execl()` 실패 (바이너리 누락) | 빌드 안내 출력 |
| `0 이상` | 게임이 직접 보낸 점수 (game3·game4·game5) | 8bit 제한 없이 그대로 기록 |
| (비어있음) + 정상 종료 | 종료코드 점수 방식 (game1·game2 의 `exit(score)`) | `WEXITSTATUS` 로 회수 (0~255) |
| (비어있음) + 비정상 종료 | 시그널 등 강제 소멸 | 비정상 종료 경고 |

game4(경영 시뮬레이션)는 메뉴 내 종료 명령이 없어 `Ctrl+C` 로 끝나는데, 스크립트가 `trap report_score SIGINT` 로 SIGINT 를 가로채 **종료 직전 자산을 점수로 환산해 파이프(argv[3])로 보냅니다.** 따라서 game4 도 game3·game5 와 같은 파이프 점수 회수 경로(②)로 순위표에 기록됩니다. 한편 게임 실행 중에는 로비가 `signal(SIGINT, SIG_IGN)` 으로 인터럽트를 잠시 무시하므로, 플레이어가 Ctrl+C 를 눌러 game4 를 끝내도 로비는 죽지 않고 메뉴로 안전하게 복귀합니다.

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

## 미니게임 2: GitHub 연동 다마고치 (game2)

2번 미니게임은 **실제 GitHub 계정의 commit 활동을 실시간으로 반영하는 다마고치**입니다. 시뮬레이션이 아니라 본인 GitHub 계정에 실제로 push 해야 다마고치의 표정이 바뀌며, 7일 연속 commit 이 없으면 다마고치가 사망합니다.

### 1. 회원가입 단계 GitHub 계정 실존 검증 (`system()` + curl)
- **호출 구조:** 회원가입 시 `src/account.c` 가 `scripts/github_check.sh` 를 `system()` 으로 호출합니다. 스크립트는 `curl` 로 GitHub REST API(`https://api.github.com/users/USER`)에 요청해 HTTP 상태코드를 받고, **200 이면 종료코드 0(존재), 404 면 1(없음)** 을 반환합니다. C 측은 `system()` 의 반환값만으로 가입 허용 여부를 판정합니다.
- **셸 인젝션 차단:** username 을 셸 인자로 넘기기 전에 C 단에서 화이트리스트 검증(`[A-Za-z0-9-]`, 1~39자, 하이픈 시작/끝 금지 — GitHub 공식 username 규칙)을 통과시켜 셸 메타문자가 스크립트에 도달할 수 없게 설계했습니다.
- **계정 저장:** 검증된 GitHub username 은 `accounts.txt` 의 3번째 필드(`id:hash:github`)에 저장되고, 로그인 시 로비가 `execl()` 의 `argv[2]` 로 game2 에 전달합니다.

### 2. 실시간 PushEvent 미러링 (`popen()` 파이프라인)
- **데이터 수집:** game2 는 `scripts/github_stats.sh` 를 `popen()` 으로 호출합니다. 스크립트는 `curl` 로 GitHub Events API(`/users/USER/events/public`)의 JSON 응답을 받아, `grep -oE` + `awk` 파이프라인으로 `"type": "PushEvent"` 와 `"created_at"` 필드만 추출합니다 (JSON 이 compact/pretty 어느 포맷이어도 매칭되도록 `:` 양옆 공백을 허용하는 정규식 사용).
- **출력 규약:** 스크립트는 stdout 으로 정확히 2줄(① 마지막 PushEvent 로부터 지난 일수, ② 응답 내 PushEvent 총 개수)만 출력하고, 네트워크 오류·curl 미설치 등 모든 실패 상황에서도 기본값(`999` / `0`)을 출력해 **C 측 `fscanf` 파싱이 절대 깨지지 않도록** 방어적으로 설계했습니다.
- **C 측 수신:** game2 는 `popen()` 으로 연 파이프에서 두 정수를 `fscanf` 로 읽고 `pclose()` 합니다. 인게임에서 `r` 키로 언제든 재조회(refresh)할 수 있습니다.

### 3. 표정(생존 상태) 결정 로직
마지막 commit 으로부터 경과한 일수와 최근 30일 push 활동량이 다마고치의 표정을 결정합니다.

| 조건                                            | 표정              |
| ----------------------------------------------- | ----------------- |
| 마지막 push 로부터 7일 이상                     | `X X` 사망        |
| 5~6일                                           | `T T` 빈사        |
| 3~4일                                           | `u u` 슬픔        |
| 1~2일 (또는 오늘 commit + 30일 push < 5)        | `o o` 보통        |
| 오늘 commit + 30일 push 5 이상                  | `^ ^` 행복        |
| 오늘 commit + 30일 push 20 이상                 | `> <` 매우행복    |
| 오늘 commit + 30일 push 50 이상                 | `\(^o^)/` 전설    |

### 4. 스크롤 없는 제자리 갱신 렌더링
- 시작 시 단 한 번만 화면 전체를 비우고(`\033[2J\033[3J\033[H`), 이후 매 프레임은 커서를 좌상단으로만 되돌려(`\033[H`) **같은 자리에 덮어쓰는 HUD 방식**으로 그립니다.
- 각 줄 끝에 `\033[K`(줄 끝까지 지우기)를 붙여 길이가 달라지는 줄의 이전 프레임 잔상을 제거하므로, 새로고침을 반복해도 터미널이 아래로 흐르지 않습니다.

### 5. 점수 공식 및 회수
```
score = (마지막 push 7일 이내 ? 100 : 0)        # 살아있음 보너스
      + max(0, 7 - days_since_commit) * 10      # 신선도 (0~70)
      + min(commits_last_30d, 100)              # 활동량 (0~100)
```
- 최대 270 → 255 로 clamp 후 `exit(score)` 로 종료하며, 로비가 `WEXITSTATUS` 폴백 경로로 회수해 순위표에 기록합니다.
- 점수를 올리는 유일한 방법은 **GitHub 에 실제로 commit 을 push** 하는 것입니다.

## 미니게임 3: 시스템 연동형 하드코어 테트리스 (game3)

3번 미니게임은 테트리스 가이드라인 물리 엔진을 구축하고, 리눅스 커널 자원 인터페이스 및 파일 시스템 환경을 프로세스 타이머 제어 루프와 동기화한 **시스템 연동형 하드코어 테트리스**입니다.

### 1. OS 호스트 CPU 자원 연동형 타이머 변조 (OS Resource Monitoring)
- **구현 원리:** POSIX 표준 익스텐션 파이프라인인 `popen()` 인터페이스를 통해 `scripts/sys_monitor.sh`를 실시간 동기식으로 호출합니다.
- **커널 데이터 파싱:** 셸 스크립트가 리눅스 커널의 `/proc` 정보에 기반한 `top` 명령어 결과에서 호스트의 실시간 CPU 사용량(%)만 유기적으로 필터링(`grep`, `awk`, `cut`)하여 정수로 뱉어내면 C 엔진이 이를 수집합니다.
- **실시간 가속 기믹:** CPU 점유율이 1% 상승할 때마다 블록의 중력 하강 주기 변수인 `drop_interval` 마이크로초를 3,500㎲씩 단축(가속)시킵니다. 시스템 부하가 클수록 속도가 폭주하는 실시간 피드백 루프를 적용했습니다.
- **자원 최적화(Throttling):** 매 프레임 파이프라인을 호출할 때 발생하는 I/O 멀티플렉싱 오버헤드를 제어하기 위해, 테트리스의 상태 이벤트 단계인 'Spawn Phase(새 블록 생성 시점)'에 맞추어 연산 주기를 조율함으로써 컨텍스트 스위칭 비용을 최소화했습니다.

### 2. 가상 파일 시스템(VFS) 및 하드웨어 오디오 비동기 제어
- **재생 메커니즘:** 인게임 이벤트(블록 고정, 라인 제거, 퍼펙트 클리어, 게임오버) 트리거 시 `system()` 시스템 콜을 호출하여 `scripts/play_sound_tetris.sh`로 전송합니다.
- **비동기 멀티태스킹:** 리눅스 환경에서 사운드 재생 유틸리티가 음원을 출력하는 동안 C 언어 메인 루프가 블로킹(화면 멈춤)되는 병목 현상을 방지하기 위해, 셸 스크립트 단에서 **백그라운드 비동기 연산자(`&`)**를 명시하여 오디오 자원을 완전 독립 구동(Non-blocking Audio)시켰습니다.

### 3. 수퍼 로테이션 시스템(SRS) 및 정밀 매트릭스 예외 제어
- **회전축 분리 설계:** 4x4 행렬 격자를 사용하는 I 블록(작대기), 회전 연산 자체가 무시되는 O 블록(네모), 그리고 3x3 중심축 격자 안에서 제자리 정형 회전을 수행하는 표준 미노들(T, S, Z, L, J)의 회전 반경 알고리즘을 분리 설계하여 그래픽 뒤틀림을 원천 방지했습니다.
- **DT포 월 킥 구현:** 정석 가이드라인의 5단계 오프셋 매트릭스(`wall_kick_data`) 좌표계를 리눅스 터미널 가상 화면 좌표계(아래로 갈수록 Y축 증가)와 수학적으로 동기화하여, 최고난도 기술인 **DT포(DT Cannon) T-스핀 트리플(T-Spin Triple) 월킥** 유격 보정을 완벽하게 가동 성공시켰습니다.
- **퍼펙트 클리어 판정:** 라인 제거 직후 보드판 전체 세그먼트의 청정 여부를 전수 스캔하는 `is_perfect_clear()` 알고리즘을 장착, 올클리어 성공 시 보너스 점수(+500점) 가산 및 전용 특수 효과음 파이프라인이 정상 트리거되도록 밸런싱했습니다.

## 미니게임 4: 자원 경영 시뮬레이션 Factory-ism (game4)

4번 미니게임은 **순수 bash 로 구현한 턴제 자원 경영 시뮬레이션 "Factory-ism"** 입니다. 석탄·철광석·철·강철 자원을 사고팔고, 광산·공장을 건설하며, 계약과 시장 가격 변동·랜덤 이벤트 속에서 자금을 불려 나갑니다.

- **모듈 구조:** 진입점은 `games/game4.sh` 하나이며, 실제 로직은 `games/management_game/` 디렉토리의 모듈 8개(`change_turn.sh`, `input.sh`, `display.sh`, `resources.sh`, `utils.sh`, `contracts_management.sh`, `market.sh`, `events.sh`)를 `source` 로 조립합니다. 모듈 경로는 `$(dirname "$0")` 기준이라 어느 위치에서 실행해도 안전합니다.
- **로비 연동:** 다른 게임과 동일하게 `fork()` + `execl()` 로 실행됩니다. bash 스크립트지만 커널의 셔뱅(`#!/bin/bash`) 해석 덕분에 바이너리와 같은 경로로 구동됩니다. 로그인 사용자 ID·GitHub username·점수 전달용 파이프 fd 를 각각 `$1`·`$2`·`$3`(`PIPE_FD`) 로 받습니다.
- **종료 및 점수 회수:** 메뉴 내 종료 명령이 없어 `Ctrl+C` 로 종료합니다. 스크립트는 `trap report_score SIGINT` 로 SIGINT 를 가로채고, 메뉴에서 정상 종료(`QUIT`)할 때도 같은 `report_score()` 를 호출하므로 **어느 경로로 끝나든 점수가 집계됩니다.** 게임 실행 동안 로비는 `SIGINT` 를 무시하므로 Ctrl+C 를 눌러도 로비 메뉴로 안전하게 복귀합니다.
- **점수 공식 (`report_score`):** 보유 자산을 가중 합산한 뒤 100 으로 나눠 점수를 만듭니다.
  ```
  FINAL_SCORE = money + coal*2 + iron_ore*3 + iron*5 + steel*10
  EXIT_SCORE  = min( FINAL_SCORE / 100 , 255 )    # 정수 나눗셈 후 255 clamp
  ```
  가공도가 높은 자원(강철 ×10, 철 ×5)일수록 점수 가중치가 커서, 단순히 현금을 쌓기보다 생산 사슬을 끝까지 돌려 고부가 자원을 비축하는 운영이 고득점으로 이어집니다.
- **파이프 직접 write (8bit 무손실):** 계산된 점수는 `write_int()` 가 **리틀엔디언 4바이트 정수**로 분해해 `printf '%b' ... >&"$PIPE_FD"` 로 파이프에 직접 씁니다. 종료코드(0~255 1바이트) 대신 파이프를 쓰므로, 로비의 하이브리드 회수 프로토콜에서 game3·game5 와 동일한 **② 파이프 점수 경로**로 회수되어 순위표에 기록됩니다.

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
├── src/                     # C source code
│   ├── account.h            # Account module interface
│   ├── account.c            # Sign-up / login / hashing / GitHub account verification
│   ├── score.h              # Score & ranking module interface
│   ├── score.c              # High-score comparison & update / time operations / leaderboard output
│   ├── game1.c              # Mini-game #1 VI-RPG source (ported from the bash original)
│   └── lobby.c              # Main menu and lobby entry point (main)
│
├── scripts/                 # Build, execution & integration shell scripts
│   ├── init.sh              # Creates data/ directory and empty files
│   ├── build.sh             # gcc compilation → bin/lobby + games/game1~3
│   ├── run.sh               # Build and execute
│   ├── clean.sh             # Cleans bin/ and game binaries (preserves data/)
│   ├── github_check.sh      # GitHub username existence check (called via system on sign-up)
│   ├── github_stats.sh      # GitHub PushEvent collection (called via popen by game2)
│   ├── play_sound_tetris.sh # Asynchronous tetris audio playback (called via system by game3)
│   └── sys_monitor.sh       # Host CPU usage parsing (called via popen by game3)
│
├── data/                    # Runtime data (excluded from git tracking)
│   ├── accounts.txt         # Account storage in id:hash:github format
│   └── scores.txt           # Score storage in game_num:username:score:time format
│
├── bin/                     # Build artifacts (excluded from git tracking)
│   └── lobby                # Compiled lobby executable
│
├── games/                   # Mini-games executed as independent processes
│   ├── game1                # #1 VI-RPG (built from src/game1.c)
│   ├── game2.c / game2      # #2 GitHub Tamagotchi (source + binary)
│   ├── game3.c / game3      # #3 VI-TETRIS (source + binary)
│   ├── game4.sh             # #4 Resource management simulation entry script (bash)
│   ├── management_game/     # Modules sourced by game4.sh
│   │   ├── change_turn.sh / input.sh / display.sh / resources.sh
│   │   └── utils.sh / contracts_management.sh / market.sh / events.sh
│   ├── game5.c / game5      # #5 Knight's Tour (source + binary)
│   └── game_bash.sh         # Original bash VI-RPG (preserved)
│
└── sound/                   # WAV resource repository for hardware audio output
    ├── game3_lock.wav       # Block locking sound
    ├── game3_clear.wav      # Line clear sound
    ├── game3_PerfectClear.wav # Perfect clear achievement sound
    └── game3_gameover.wav   # Game over sound
```

## Directory Roles

| Directory   | Purpose |
| ---------- | ------------------------------------------ |
| `src/`     | C source/header files. Contains all implementation code |
| `scripts/` | Shell scripts (build, run, initialize, clean) |
| `data/`    | User data storage (accounts, etc.). Local-only |
| `games/`   | Games executed as independent processes (binaries + game4.sh script + modules) |
| `bin/`     | gcc build artifacts. Removed by `make clean` |
| `sound/`     | Audio subsystem resource directory |


## Libraries Used

Only the C standard library is used:

- `<stdio.h>`  — File I/O (`fopen`, `fprintf`, `fgets`, etc.)
- `<stdlib.h>` — `system()`, `strtoul`, `atoi`
- `<string.h>` — String processing
- `<time.h>`   — System time operations and formatting (`time`, `localtime`, `strftime`)
- `<unistd.h>` — POSIX OS API interface. File descriptor manipulation and process image replacement (`read`, `close`, `execl`, `STDIN_FILENO`)
- `<sys/select.h>` — I/O multiplexing system call. Handles microsecond-level asynchronous keyboard input timeouts (`select`)
- `<sys/wait.h>` — Child process lifecycle management and kernel signal tracing interface (`wait`)
- `<termios.h>` — Terminal I/O configuration interface. Enables unbuffered input (Raw Mode) and echo suppression (`tcgetattr`, `tcsetattr`)

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
id:hashvalue:github_username
```

One account per line. Passwords are stored using a modified djb2 hash combined with username-based salting.
The github field holds a GitHub username verified to exist via `scripts/github_check.sh` at sign-up (used by game2).

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

The authenticated user's ID (`argv[1]`), GitHub username (`argv[2]`), and the score pipe fd (`argv[3]`) are securely passed as program arguments. game4 is a bash script (`games/game4.sh`) rather than a compiled binary, but the kernel's shebang (`#!/bin/bash`) handling lets it run through the very same `execl()` path.

#### 2. Missing Executable Exception Handling Using `pipe`

To handle situations where `execl()` fails (for example, when the game binary is missing or not compiled), an anonymous pipe (`pipe`) is created immediately before `fork()`.

If `execl()` fails, the child process writes an error signal (`-1`) into the pipe and exits immediately. Since scores are always non-negative, `-1` is a safe failure-only sentinel.

The parent process monitors this pipe signal to accurately detect missing game files. This completely prevents collision bugs where a legitimate game termination could mistakenly be interpreted as an execution failure.

#### 3. Hybrid Score Retrieval Protocol (pipe first, exit-code fallback)

After `wait(&status)` returns, the parent recovers the score in the following order:

| Pipe value received | Meaning | Handling |
| --- | --- | --- |
| `-1` | `execl()` failure (missing binary) | Print build instructions |
| `>= 0` | Score sent directly by the game (game3/game4/game5) | Recorded as-is, no 8-bit limit |
| (empty) + normal exit | Exit-code scoring (`exit(score)` in game1/game2) | Recovered via `WEXITSTATUS` (0–255) |
| (empty) + abnormal exit | Killed by a signal | Abnormal-termination warning |

game4 (management simulation) has no in-menu quit command, so it ends on `Ctrl+C`; the script installs `trap report_score SIGINT`, converting its assets into a score and writing it to the pipe (`argv[3]`) just before exit. game4 is therefore retrieved through the same pipe-score path (②) as game3/game5. Meanwhile, while a game is running, the lobby temporarily ignores `SIGINT` (`signal(SIGINT, SIG_IGN)`) so that quitting game4 with Ctrl+C does not kill the lobby itself.

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

## Mini-Game 3: OS Resource-Linked Hardcore Tetris (game3)

The third mini-game is a high-performance Tetris game built on a precise implementation of the Tetris Guideline physics engine. It serves as a system-programming-intensive framework that synchronizes Linux kernel resource interfaces and the virtual file system with the process timer control loop.

### 1. Host OS CPU Resource-Linked Timer Modulation (OS Resource Monitoring)
- **Implementation Mechanism:** Establishes a real-time synchronous pipeline by calling `scripts/sys_monitor.sh` through the POSIX standard extension `popen()` interface.
- **Kernel Data Parsing:** The shell script monitors and filters (`grep`, `awk`, `cut`) the real-time host CPU usage percentage from the `top` command utilities based on the Linux kernel's `/proc` file system, returning it as a clean integer to the C engine.
- **Real-Time Acceleration:** For every 1% increase in host CPU utilization, the block's gravity drop timer variable (`drop_interval`) is shortened (accelerated) by 3,500 microseconds. This demonstrates a real-time feedback loop where the gameplay speed dynamically intensifies under high system loads.
- **Resource Optimization (Throttling):** To prevent system overhead and I/O multiplexing latency caused by polling the kernel on every frame, the execution frequency is throttled to synchronize strictly with the in-game 'Spawn Phase' (the moment a new block is generated), minimizing unnecessary context-switching costs.

### 2. Virtual File System (VFS) & Asynchronous Hardware Audio Control
- **Playback Architecture:** When key in-game events occur (block locking, line clearing, perfect clearing, or game over), a signal is dispatched via the `system()` call to execute `scripts/play_sound_tetris.sh`.
- **Asynchronous Multitasking:** To prevent the C standard execution loop from blocking (causing screen lag) while the Linux audio player utility processes sound output, the shell script employs the **background asynchronous operator (`&`)**. This achieves fully independent, non-blocking audio thread execution.

### 3. Super Rotation System (SRS) & Precise Matrix Exception Handling
- **Decoupled Rotation Axes:** Tailors distinct rotation algorithms for the 4x4 matrix-based I-block (Long bar), the rotation-exempt O-block (Square), and the standard 3x3 center-axis tetrominoes (T, S, Z, L, J), completely preventing graphical fragmentation.
- **DT Cannon Wall Kicks:** Mathematical synchronization is established between the 5-step SRS offset matrix (`wall_kick_data`) and the Linux terminal virtual display coordinate system (where the Y-axis increases downwards). This enables flawless wall kick execution for the advanced **DT Cannon Opening: T-Spin Double to T-Spin Triple** sequence.

# VI-RPG

bash로 구현한 터미널 기반 RPG 미니게임.
13x21 크기의 던전에서 고블린과 보스를 처치하며 진행한다.

## 실행 방법

프로젝트 루트에서:

```sh
bash games/game_bash.sh
```

또는 실행 권한을 준 뒤:

```sh
chmod +x games/game_bash.sh
./games/game_bash.sh
```

## 조작법

| 키       | 동작                  |
| -------- | --------------------- |
| `w`      | 위로 이동             |
| `a`      | 왼쪽 이동             |
| `s`      | 아래로 이동           |
| `d`      | 오른쪽 이동           |
| `SPACE`  | 인접 4방향 공격       |
| `e`      | 주변 3x3 범위 스킬    |
| `q`      | 게임 종료             |

## 타일 설명

| 기호    | 의미                  |
| ------- | --------------------- |
| `♥`     | 플레이어              |
| `1/2/3` | 고블린 (HP 3)         |
| `B`     | 보스 (HP 10)          |
| `H`     | 상점 (HP 회복)        |
| `♣ / ♠` | 나무 (통과 불가)      |
| `█`     | 벽 (통과 불가)        |
| `.`     | 이동 가능한 빈 칸     |

## 게임 규칙

- 초기 스탯: HP 5, ATK 2
- 고블린 접촉 시 HP -1, 보스 접촉 시 HP -2
- 몬스터 처치 시 골드 +5
- 레벨업: 1킬, 3킬, 6킬 달성 시 ATK 증가
- 보스(B)를 처치하면 게임 클리어
- HP가 0이 되면 게임 오버

---

# GitHub Tamagotchi (game2)

실제 GitHub 공개 활동을 popen() 으로 받아 다마고치 표정에 반영하는 미니게임.
시뮬레이션 commit 이 아니라 **본인 GitHub 계정에 실제로 push 해야** 표정이 바뀐다.

## 의존성

- `curl` (Rocky Linux 기본 또는 `dnf install curl`)
- `date -d` (GNU date - Linux 표준)
- 인터넷 연결 (GitHub REST API 호출)

## 데이터 소스

- `scripts/github_stats.sh USER` 가 `https://api.github.com/users/USER/events/public` 에서
  PushEvent 들을 긁어 두 숫자를 stdout 으로 출력:
  - line 1: 마지막 PushEvent 로부터 지난 일수 (없으면 999)
  - line 2: 응답에 포함된 PushEvent 총 개수 (GitHub Events API 한도 내)
- game2 가 popen() 으로 그 두 줄을 fscanf 한다.
- 회원가입(register) 단계에서는 `scripts/github_check.sh` 를 system() 으로 호출해
  username 존재 여부만 검증한다 (HTTP 200 / 404).

## 빌드 & 실행

```sh
sh scripts/build.sh     # games/game2 가 함께 빌드됨
./games/game2 사용자ID  # 단독 실행
```

로비(`bin/lobby`)에서 메뉴 `2` 를 선택하면 fork/exec 로 자동 실행되고,
종료 시 점수가 `WEXITSTATUS` 로 회수되어 최고점수 파일에 기록된다.

## 조작법

| 키  | 동작                                |
| --- | ----------------------------------- |
| `r` | refresh (GitHub 다시 조회)          |
| `q` | 현재 점수로 종료                    |

## 표정 단계

| 조건                                            | 표정              |
| ----------------------------------------------- | ----------------- |
| 마지막 push 로부터 7일 이상                     | `X X` 사망        |
| 5~6일                                           | `T T` 빈사        |
| 3~4일                                           | `u u` 슬픔        |
| 1~2일 (또는 오늘 commit + 30일 push < 5)        | `o o` 보통        |
| 오늘 commit + 30일 push 5 이상                  | `^ ^` 행복        |
| 오늘 commit + 30일 push 20 이상                 | `> <` 매우행복    |
| 오늘 commit + 30일 push 50 이상                 | `\(^o^)/` 전설    |

## 점수 공식

```
score = (마지막 push 7일 이내 ? 100 : 0)        # 살아있음 보너스
      + max(0, 7 - days_since_commit) * 10      # 신선도 (0~70)
      + min(commits_last_30d, 100)              # 활동량 (0~100)
```

- 최대 270 → 255 로 clamp (exit code 8bit 제한)
- 점수를 올리려면 **GitHub 에 실제로 push** 후 `r` 로 새로고침 또는 게임 재실행

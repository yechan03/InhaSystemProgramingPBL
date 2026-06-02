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

C 표준 라이브러리만으로 구현한 “GitHub 다마고치 키우기” 미니게임.
매일 commit 을 해줘야 다마고치가 행복하게 살아간다.

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
| `c` | commit (streak +1, HP/Mood 회복)    |
| `s` | skip (미 commit 일수 +1, HP/Mood -) |
| `q` | 현재 점수로 종료                    |

## 표정 단계

| 조건                            | 표정          |
| ------------------------------- | ------------- |
| 7일 연속 미 commit              | `X X` 사망    |
| 5~6일 미 commit                 | `T T` 빈사    |
| 3~4일 미 commit                 | `u u` 슬픔    |
| 0~2일 미 commit & streak < 3    | `o o` 보통    |
| streak 3 이상                   | `^ ^` 행복    |
| streak 7 이상                   | `> <` 매우행복|
| streak 14 이상                  | `\(^o^)/` 전설|

## 게임 규칙

- 초기 HP 10, Mood 5
- `c` commit  : streak +1, days_since_commit 0, HP +1, Mood +2
- `s` skip    : streak = 0, days_since_commit +1, HP -1, Mood -2
- **7일 연속 미 commit** 이면 다마고치 사망 → 게임 종료
- 3일 이상 미 commit 부터 표정이 점점 슬퍼짐
- `q` 로 언제든 살아있는 상태로 게임 종료 가능
- 최종 점수 = `total_commits * 2 + max_streak * 3 + days_lived`
- 점수는 0~255 범위로 clamp 되어 로비에 반환 (exit code 8bit 제한)

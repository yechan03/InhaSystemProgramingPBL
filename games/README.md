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




# VI-RPG

A terminal-based RPG mini-game implemented in `bash`.  
Progress through a 13x21 dungeon by defeating goblins and the boss.

## How to Run

From the project root:

```sh
bash games/game_bash.sh
```

Or grant execute permission first:

```sh
chmod +x games/game_bash.sh
./games/game_bash.sh
```

## Controls

| Key      | Action                  |
| -------- | ----------------------- |
| `w`      | Move up                 |
| `a`      | Move left               |
| `s`      | Move down               |
| `d`      | Move right              |
| `SPACE`  | Attack adjacent 4 directions |
| `e`      | Area skill (3x3 range)  |
| `q`      | Quit game               |

## Tile Description

| Symbol   | Meaning                 |
| -------- | ----------------------- |
| `♥`      | Player                  |
| `1/2/3`  | Goblin (HP 3)           |
| `B`      | Boss (HP 10)            |
| `H`      | Shop (HP recovery)      |
| `♣ / ♠`  | Trees (impassable)      |
| `█`      | Wall (impassable)       |
| `.`      | Empty traversable tile  |

## Game Rules

- Initial stats: HP 5, ATK 2
- Contact with a goblin: HP -1
- Contact with the boss: HP -2
- Gold +5 for each monster defeated
- Level-up ATK increases after reaching 1, 3, and 6 kills
- Defeat the boss (`B`) to clear the game
- Game over when HP reaches 0

#!/bin/bash
# ============================================================
#  VI-RPG : 리눅스 VI 에디터 RPG 게임
#  sh rpg_game.sh  또는  bash rpg_game.sh 로 실행
# ============================================================
# sh로 실행될 경우 자동으로 bash로 재실행
if [ -z "$BASH_VERSION" ]; then
    exec bash "$0" "$@"
fi

# ──────────────── ANSI 색상 (ESC 직접 정의 - sh 호환) ────────────────
ESC=$(printf '\033')
R="${ESC}[31m"    BR="${ESC}[91m"   G="${ESC}[32m"   BG="${ESC}[92m"
Y="${ESC}[33m"    BY="${ESC}[93m"   C="${ESC}[36m"   BC="${ESC}[96m"
M="${ESC}[35m"    BM="${ESC}[95m"   W="${ESC}[37m"   BW="${ESC}[97m"
GR="${ESC}[90m"   BOLD="${ESC}[1m"  RST="${ESC}[0m"
BG_Y="${ESC}[43m" BG_R="${ESC}[41m"

# 색상 래퍼
c()  { echo -n "${1}${2}${RST}"; }
cl() { echo "${1}${2}${RST}"; }

# ──────────────── 화면 제어 ────────────────
clear_screen() { echo "${ESC}[2J${ESC}[H"; }

# ──────────────── 게임 상수 ────────────────
PLAYER_MAX_HP=5
PLAYER_ATK=2
GOBLIN_MAX_HP=3
BOSS_MAX_HP=10
GOLD_REWARD=5
MAP_ROWS=13
MAP_COLS=21

# ──────────────── 전역 상태 ────────────────
PLAYER_ROW=0; PLAYER_COL=0
PLAYER_HP=$PLAYER_MAX_HP
PLAYER_ATTACK=$PLAYER_ATK
PLAYER_GOLD=0
PLAYER_LEVEL=1
TURN=0
KILL_COUNT=0
LAST_MSG=""
SKILL_HITS=0
BOSS_DEAD=0

# 레벨업 테이블: "킬수:공격력보너스"
LEVEL_TABLE="1:1 3:1 6:2"

# 맵 / HP 그리드
declare -a BOARD
declare -a GOBLIN_HP
declare -a BOSS_HP

# ──────────────── 인덱스 헬퍼 ────────────────
idx()     { echo $(( $1 * MAP_COLS + $2 )); }
get_tile(){ local i; i=$(idx $1 $2); echo "${BOARD[$i]:-#}"; }
set_tile(){ local i; i=$(idx $1 $2); BOARD[$i]="$3"; }
get_ghp() { local i; i=$(idx $1 $2); echo "${GOBLIN_HP[$i]:-0}"; }
set_ghp() { local i; i=$(idx $1 $2); GOBLIN_HP[$i]=$3; }
get_bhp() { local i; i=$(idx $1 $2); echo "${BOSS_HP[$i]:-0}"; }
set_bhp() { local i; i=$(idx $1 $2); BOSS_HP[$i]=$3; }

# ──────────────── 맵 생성 ────────────────
generate_map() {
    local r c i

    # 초기화
    for (( r=0; r<MAP_ROWS; r++ )); do
        for (( c=0; c<MAP_COLS; c++ )); do
            i=$(idx $r $c)
            if (( r==0 || r==MAP_ROWS-1 || c==0 || c==MAP_COLS-1 )); then
                BOARD[$i]='#'
            else
                BOARD[$i]='.'
            fi
            GOBLIN_HP[$i]=0
            BOSS_HP[$i]=0
        done
    done

    # 장애물 랜덤 배치
    local obstacles=$(( RANDOM % 11 + 15 ))
    local placed=0 attempts=0 rnd
    while (( placed < obstacles && attempts < 500 )); do
        r=$(( RANDOM % (MAP_ROWS-2) + 1 ))
        c=$(( RANDOM % (MAP_COLS-2) + 1 ))
        i=$(idx $r $c)
        if [[ "${BOARD[$i]}" == '.' ]]; then
            rnd=$(( RANDOM % 4 ))
            if   (( rnd == 0 || rnd == 1 )); then BOARD[$i]='#'
            elif (( rnd == 2 ));              then BOARD[$i]='T'
            else                                   BOARD[$i]='t'
            fi
            (( placed++ ))
        fi
        (( attempts++ ))
    done

    # 플레이어 배치 (왼쪽 상단 첫 빈칸)
    PLAYER_ROW=-1; PLAYER_COL=-1
    for (( r=1; r<MAP_ROWS-1; r++ )); do
        for (( c=1; c<MAP_COLS-1; c++ )); do
            i=$(idx $r $c)
            if [[ "${BOARD[$i]}" == '.' && $PLAYER_ROW == -1 ]]; then
                PLAYER_ROW=$r; PLAYER_COL=$c
            fi
        done
    done

    # 아이템/몬스터 배치
    place_char() {
        local ch=$1 count=$2 p=0 att=0
        while (( p < count && att < 1000 )); do
            r=$(( RANDOM % (MAP_ROWS-2) + 1 ))
            c=$(( RANDOM % (MAP_COLS-2) + 1 ))
            i=$(idx $r $c)
            if [[ "${BOARD[$i]}" == '.' ]] && ! (( r==PLAYER_ROW && c==PLAYER_COL )); then
                BOARD[$i]="$ch"
                if [[ "$ch" == '1' || "$ch" == '2' || "$ch" == '3' ]]; then
                    GOBLIN_HP[$i]=$GOBLIN_MAX_HP
                fi
                if [[ "$ch" == 'B' ]]; then
                    BOSS_HP[$i]=$BOSS_MAX_HP
                fi
                (( p++ ))
            fi
            (( att++ ))
        done
    }

    place_char 'H' 2
    place_char '1' 1
    place_char '2' 1
    place_char '3' 1
    place_char 'B' 1
}

# ──────────────── HP 바 ────────────────
hp_bar() {
    local cur=$1 max=$2 col=$3
    local filled=$(( cur * 10 / max ))
    (( filled < 0 )) && filled=0
    (( filled > 10 )) && filled=10
    local bar="" i
    for (( i=0; i<10; i++ )); do
        if (( i < filled )); then
            bar="${bar}${col}█${RST}"
        else
            bar="${bar}${GR}░${RST}"
        fi
    done
    echo -n "[${bar}]"
}

# ──────────────── 타일 색상 렌더 ────────────────
colored_tile() {
    local tile=$1 hi=$2
    if [[ "$hi" == "1" ]]; then
        echo -n "${BG_Y}${BOLD}${BR}*${RST}"
        return
    fi
    case "$tile" in
        '#') echo -n "${GR}█${RST}" ;;
        'T') echo -n "${G}♣${RST}" ;;
        't') echo -n "${BG}♠${RST}" ;;
        'H') echo -n "${BC}H${RST}" ;;
        '1') echo -n "${BR}1${RST}" ;;
        '2') echo -n "${BR}2${RST}" ;;
        '3') echo -n "${BR}3${RST}" ;;
        'B') echo -n "${BOLD}${M}B${RST}" ;;
        '.') echo -n "${GR}.${RST}" ;;
        *)   echo -n "$tile" ;;
    esac
}

# ──────────────── 몬스터 HP 조회 ────────────────
find_char_hp() {
    local ch=$1 r c i
    for (( r=0; r<MAP_ROWS; r++ )); do
        for (( c=0; c<MAP_COLS; c++ )); do
            i=$(idx $r $c)
            if [[ "${BOARD[$i]}" == "$ch" ]]; then
                if [[ "$ch" == 'B' ]]; then
                    echo "${BOSS_HP[$i]:-0}"; return
                fi
                echo "${GOBLIN_HP[$i]:-0}"; return
            fi
        done
    done
    echo "0"
}

# ──────────────── UI 박스 출력 ────────────────
BOX_W=46

box_sep() {
    echo -n "  ${BOLD}${C}"
    local i; for (( i=0; i<BOX_W+2; i++ )); do printf '─'; done
    echo "$RST"
}

# box_line: 내용을 그대로 출력하고 오른쪽을 패딩
# $1=내용(색상포함), $2=보이는 글자 수(수동)
box_line() {
    local content="$1"
    echo -n "  "
    echo -n "$content"
    echo ""
}

# ──────────────── 렌더링 ────────────────
render() {
    local skill_cells="$1"
    clear_screen

    # 헤더
    box_sep
    local hdr_txt="       >>>  VI-RPG : VI 에디터 던전  <<<"
    box_line "${BOLD}${BY}${hdr_txt}${RST}" ${#hdr_txt}
    box_sep
    echo ""

    # 맵 출력
    echo "${BOLD}${W}  [ 게임 맵 ]${RST}"
    local r c tile hi cell
    for (( r=0; r<MAP_ROWS; r++ )); do
        echo -n "  "
        for (( c=0; c<MAP_COLS; c++ )); do
            tile=$(get_tile $r $c)
            hi=0
            if [[ -n "$skill_cells" ]]; then
                for cell in $skill_cells; do
                    if [[ "$cell" == "${r},${c}" ]]; then hi=1; break; fi
                done
            fi
            if (( r == PLAYER_ROW && c == PLAYER_COL )); then
                echo -n "${BOLD}${BY}♥${RST}"
            else
                colored_tile "$tile" "$hi"
            fi
            echo -n " "
        done
        echo ""
    done
    echo ""

    # ── 플레이어 상태 ──
    box_sep
    local hp_col
    if (( PLAYER_HP > 2 )); then hp_col="$BG"; else hp_col="$BR"; fi
    local hbar; hbar=$(hp_bar "$PLAYER_HP" "$PLAYER_MAX_HP" "$hp_col")

    # stat1: TURN / HP / bar
    local lv_idx=$(( PLAYER_LEVEL - 1 ))
    local lv_info="" entry needed bonus
    local idx_cnt=0
    for entry in $LEVEL_TABLE; do
        if (( idx_cnt == lv_idx )); then
            needed="${entry%%:*}"; lv_info="Kill:${KILL_COUNT}/${needed}"; break
        fi
        (( idx_cnt++ ))
    done
    [[ -z "$lv_info" ]] && lv_info="MAX LV"

    local turn_str
    turn_str="TURN $(printf "%3d" "$TURN")"

    # stat1 보이는 길이: " TURN xxx  HP:x/5 [bar10]" = 1+8+2+3+hp_len+1+max_len+1+12
    local hp_vlen=$(( 1 + 8 + 2 + 3 + ${#PLAYER_HP} + 1 + ${#PLAYER_MAX_HP} + 1 + 12 ))
    local stat1
    stat1=" ${BOLD}${turn_str}${RST}  HP:${hp_col}${PLAYER_HP}${RST}/${PLAYER_MAX_HP} "
    stat1="${stat1}${hbar}"
    box_line "$stat1" "$hp_vlen"

    # stat2: Lv / ATK / Gold / kill info
    local lv_col; (( PLAYER_LEVEL >= 3 )) && lv_col="$BY" || lv_col="$Y"
    local stat2_vlen=$(( 4 + ${#PLAYER_LEVEL} + 2 + 4 + ${#PLAYER_ATTACK} + 2 + 5 + ${#PLAYER_GOLD} + 2 + ${#lv_info} ))
    local stat2
    stat2=" ${lv_col}Lv:${PLAYER_LEVEL}${RST}  ${BY}ATK:${PLAYER_ATTACK}${RST}  ${Y}Gold:${PLAYER_GOLD}${RST}  ${GR}${lv_info}${RST}"
    box_line "$stat2" "$stat2_vlen"

    # ── 몬스터 HP ──
    box_sep
    local boss_hp g1_hp g2_hp g3_hp
    boss_hp=$(find_char_hp 'B')
    g1_hp=$(find_char_hp '1')
    g2_hp=$(find_char_hp '2')
    g3_hp=$(find_char_hp '3')

    local b_col; (( boss_hp > BOSS_MAX_HP/2 )) && b_col="$M" || b_col="$BR"
    local bbar; bbar=$(hp_bar "$boss_hp" "$BOSS_MAX_HP" "$b_col")
    local boss_vlen=$(( 5 + 2 + 1 + ${#BOSS_MAX_HP} + 1 + 12 ))
    local boss_line
    boss_line=" ${BOLD}${M}BOSS${RST} ${b_col}${boss_hp}${RST}/${BOSS_MAX_HP} "
    boss_line="${boss_line}${bbar}"
    box_line "$boss_line" "$boss_vlen"

    local gvlen=$(( 3+${#g1_hp}+1+${#GOBLIN_MAX_HP} + 2 + 3+${#g2_hp}+1+${#GOBLIN_MAX_HP} + 2 + 3+${#g3_hp}+1+${#GOBLIN_MAX_HP} + 1 ))
    local gline
    gline=" ${BR}G1:${g1_hp}/${GOBLIN_MAX_HP}${RST}  ${BR}G2:${g2_hp}/${GOBLIN_MAX_HP}${RST}  ${BR}G3:${g3_hp}/${GOBLIN_MAX_HP}${RST}"
    box_line "$gline" "$gvlen"

    # ── 조작 안내 ──
    box_sep
    box_line " ${BOLD}${W}[ 조작키 & 타일 설명 ]${RST}" 22
    box_line " ${BY}♥${RST}=나  ${BR}1/2/3${RST}=고블린  ${M}B${RST}=보스  ${BC}H${RST}=상점" 31
    box_line " ${G}♣${RST}=큰나무  ${BG}♠${RST}=작은나무  ${GR}█${RST}=벽(통과불가)" 30
    box_sep
    box_line " 이동: ${BY}w${RST}(위) ${BY}a${RST}(왼) ${BY}s${RST}(아래) ${BY}d${RST}(오른)" 33
    box_line " 공격: ${BY}SPACE${RST}  범위스킬: ${BY}e${RST}  종료: ${BY}q${RST}" 30
    box_sep

    # ── 메시지 ──
    if [[ -n "$LAST_MSG" ]]; then
        echo ""
        box_sep
        box_line "$LAST_MSG" 30
        box_sep
    fi
}

# ──────────────── 이동 (wasd) ────────────────
do_move() {
    local key=$1
    local nr=$PLAYER_ROW nc=$PLAYER_COL

    case "$key" in
        w) (( nr-- )) ;;   # 위
        s) (( nr++ )) ;;   # 아래
        a) (( nc-- )) ;;   # 왼쪽
        d) (( nc++ )) ;;   # 오른쪽
        *) return ;;
    esac

    # 경계 체크
    (( nr < 0 || nr >= MAP_ROWS || nc < 0 || nc >= MAP_COLS )) && return

    local tile; tile=$(get_tile $nr $nc)

    # 벽/나무 통과 불가
    case "$tile" in
        '#'|'T'|'t') return ;;
    esac

    # 몬스터/상점 접촉 처리
    case "$tile" in
        '1'|'2'|'3')
            (( PLAYER_HP-- ))
            LAST_MSG=" ${BR}! 고블린에게 피해를 입었습니다! HP-1${RST}"
            ;;
        'B')
            (( PLAYER_HP -= 2 ))
            LAST_MSG=" ${BR}!! 보스에게 피해를 입었습니다! HP-2${RST}"
            ;;
        'H')
            local heal=$(( PLAYER_MAX_HP - PLAYER_HP ))
            (( heal > GOLD_REWARD )) && heal=$GOLD_REWARD
            if (( heal > 0 )); then
                (( PLAYER_HP += heal ))
                LAST_MSG=" ${BC}+ 상점에서 HP ${heal} 회복!${RST}"
            else
                LAST_MSG=" ${GR}이미 HP가 최대입니다.${RST}"
            fi
            ;;
    esac

    PLAYER_ROW=$nr
    PLAYER_COL=$nc
}

# ──────────────── 일반 공격 (SPACE) ────────────────
do_attack() {
    local r c tile hp
    local dirs="-1,0 1,0 0,-1 0,1"
    local dir dr dc
    for dir in $dirs; do
        dr="${dir%%,*}"; dc="${dir##*,}"
        r=$(( PLAYER_ROW + dr ))
        c=$(( PLAYER_COL + dc ))
        (( r<0 || r>=MAP_ROWS || c<0 || c>=MAP_COLS )) && continue
        tile=$(get_tile $r $c)
        case "$tile" in
            '1'|'2'|'3')
                hp=$(get_ghp $r $c)
                if (( hp > 0 )); then
                    (( hp -= PLAYER_ATTACK ))
                    (( hp < 0 )) && hp=0
                    set_ghp $r $c $hp
                fi
                ;;
            'B')
                hp=$(get_bhp $r $c)
                if (( hp > 0 )); then
                    (( hp -= PLAYER_ATTACK ))
                    (( hp < 0 )) && hp=0
                    set_bhp $r $c $hp
                fi
                ;;
        esac
    done
}

# ──────────────── 범위 스킬 (e) ────────────────
do_skill() {
    SKILL_HITS=0
    local skill_dmg=$(( PLAYER_ATTACK / 2 ))
    (( skill_dmg < 1 )) && skill_dmg=1

    # 주변 8칸 목록
    local cells="" dr dc r c
    for dr in -1 0 1; do
        for dc in -1 0 1; do
            (( dr==0 && dc==0 )) && continue
            r=$(( PLAYER_ROW + dr ))
            c=$(( PLAYER_COL + dc ))
            (( r<0 || r>=MAP_ROWS || c<0 || c>=MAP_COLS )) && continue
            cells="${cells}${r},${c} "
        done
    done

    # 1단계: 범위 표시
    LAST_MSG=" ${BOLD}${BY}** 스킬 범위 표시중... **${RST}"
    render "$cells"
    sleep 0.4

    # 2단계: 데미지 적용
    local tile hp cell
    for cell in $cells; do
        r="${cell%%,*}"; c="${cell##*,}"
        tile=$(get_tile $r $c)
        case "$tile" in
            '1'|'2'|'3')
                hp=$(get_ghp $r $c)
                if (( hp > 0 )); then
                    (( hp -= skill_dmg )); (( hp < 0 )) && hp=0
                    set_ghp $r $c $hp; (( SKILL_HITS++ ))
                fi
                ;;
            'B')
                hp=$(get_bhp $r $c)
                if (( hp > 0 )); then
                    (( hp -= skill_dmg )); (( hp < 0 )) && hp=0
                    set_bhp $r $c $hp; (( SKILL_HITS++ ))
                fi
                ;;
        esac
    done
}

# ──────────────── 죽은 몬스터 처리 ────────────────
check_dead() {
    BOSS_DEAD=0
    local r c i tile hp
    for (( r=0; r<MAP_ROWS; r++ )); do
        for (( c=0; c<MAP_COLS; c++ )); do
            i=$(idx $r $c)
            tile="${BOARD[$i]}"
            case "$tile" in
                '1'|'2'|'3')
                    hp="${GOBLIN_HP[$i]:-0}"
                    if (( hp <= 0 )); then
                        BOARD[$i]='.'
                        (( PLAYER_GOLD += GOLD_REWARD ))
                        (( KILL_COUNT++ ))
                    fi
                    ;;
                'B')
                    hp="${BOSS_HP[$i]:-0}"
                    if (( hp <= 0 )); then
                        BOARD[$i]='.'
                        (( PLAYER_GOLD += GOLD_REWARD ))
                        (( KILL_COUNT++ ))
                        BOSS_DEAD=1
                    fi
                    ;;
            esac
        done
    done
}

# ──────────────── 레벨업 체크 ────────────────
check_levelup() {
    local lv_idx=$(( PLAYER_LEVEL - 1 ))
    local idx_cnt=0 entry needed bonus
    for entry in $LEVEL_TABLE; do
        if (( idx_cnt == lv_idx )); then
            needed="${entry%%:*}"; bonus="${entry##*:}"
            if (( KILL_COUNT >= needed )); then
                (( PLAYER_LEVEL++ ))
                (( PLAYER_ATTACK += bonus ))
                return 0
            fi
            break
        fi
        (( idx_cnt++ ))
    done
    return 1
}

# ──────────────── 엔딩 화면 ────────────────
show_gameover() {
    echo ""
    cl "${BOLD}${BR}" "  +========================================+"
    cl "${BOLD}${BR}" "  |     플레이어가 사망했습니다...          |"
    cl "${BOLD}${BR}" "  |           -- 게임  오버 --              |"
    cl "${BOLD}${BR}" "  +========================================+"
    echo ""
}

show_clear() {
    echo ""
    cl "${BOLD}${BY}" "  +========================================+"
    cl "${BOLD}${BY}" "  |   !!!  보스를 처치했습니다!!  !!!      |"
    cl "${BOLD}${BY}" "  |        *** 게임  클리어! ***           |"
    cl "${BOLD}${BY}" "  +========================================+"
    echo ""
}

# ──────────────── 타이틀 화면 ────────────────
show_title() {
    clear_screen
    echo ""
    cl "${BOLD}${BY}" "  ╔══════════════════════════════════════════╗"
    cl "${BOLD}${BY}" "  ║         VI-RPG : VI RPG 던전             ║"
    cl "${BOLD}${BY}" "  ╚══════════════════════════════════════════╝"
    echo ""
    cl "${C}"  "  ┌─────────────────────────────────────────┐"
    cl "${C}"  "  │  이동   :  w(위)  a(왼)  s(아래)  d(오) │"
    cl "${C}"  "  │  공격   :  SPACE  (인접 4방향 공격)     │"
    cl "${C}"  "  │  스킬   :  e      (3x3 범위 공격)       │"
    cl "${C}"  "  │  종료   :  q                            │"
    cl "${C}"  "  ├─────────────────────────────────────────┤"
    cl "${C}"  "  │  ♥=나  1/2/3=고블린  B=보스  H=상점     │"
    cl "${C}"  "  │  ♣/♠=나무(벽)  █=벽  .=이동가능         │"
    cl "${C}"  "  └─────────────────────────────────────────┘"
    echo ""
    echo "  ${BY}엔터를 누르면 시작합니다... let's go!${RST}"
    IFS= read -r _dummy
}

# ──────────────── 키 읽기 ────────────────
read_key() {
    local key="" OLD_STTY
    OLD_STTY=$(stty -g 2>/dev/null)
    stty cbreak -echo 2>/dev/null
    IFS= read -r -s -n1 key
    stty "$OLD_STTY" 2>/dev/null

    case "$key" in
        " ")    echo "space" ;;
        ""|\
$'\n'|\
$'\r')  echo "enter" ;;
        *)      echo "$key"  ;;
    esac
}

# ──────────────── 메인 게임 루프 ────────────────
game_loop() {
    generate_map
    PLAYER_HP=$PLAYER_MAX_HP
    PLAYER_ATTACK=$PLAYER_ATK
    PLAYER_GOLD=0
    PLAYER_LEVEL=1
    TURN=0
    KILL_COUNT=0
    LAST_MSG=""

    render ""

    local key sdmg
    while true; do
        key=$(read_key)
        LAST_MSG=""

        case "$key" in
            q|Q)
                echo ""
                echo "  ${BY}  게임을 종료합니다. 안녕히 가세요!${RST}"
                break
                ;;

            # ── 이동: wasd ──
            w|a|s|d)
                do_move "$key"
                ;;

            # ── 공격: SPACE ──
            space)
                do_attack
                check_dead
                LAST_MSG=" ${BY}>> 공격! (SPACE - 상하좌우 4방향)${RST}"
                if check_levelup; then
                    LAST_MSG=" ${BOLD}${BY}*** LEVEL UP! Lv${PLAYER_LEVEL}  ATK:${PLAYER_ATTACK} ***${RST}"
                fi
                if (( BOSS_DEAD )); then
                    render ""
                    show_clear
                    break
                fi
                ;;

            # ── 범위 스킬: e ──
            e|E)
                do_skill
                check_dead
                sdmg=$(( PLAYER_ATTACK / 2 ))
                (( sdmg < 1 )) && sdmg=1
                LAST_MSG=" ${BOLD}${C}** 범위 스킬! ${SKILL_HITS}칸 적중! (DMG:${sdmg})${RST}"
                if check_levelup; then
                    LAST_MSG=" ${BOLD}${BY}*** LEVEL UP! Lv${PLAYER_LEVEL}  ATK:${PLAYER_ATTACK} ***${RST}"
                fi
                if (( BOSS_DEAD )); then
                    render ""
                    show_clear
                    break
                fi
                ;;
        esac

        (( TURN++ ))
        render ""

        if (( PLAYER_HP <= 0 )); then
            show_gameover
            break
        fi
    done
}

# ──────────────── 진입점 ────────────────
show_title
game_loop

# Hello World 테스트 — C & 셸 스크립트

**VSCode → GitHub → VMware (Rocky Linux)** 작업 흐름을 검증하는 간단한 연습.

## 목표
Windows의 VSCode에서 코드 작성 → GitHub에 push → VMware에서 pull → 컴파일 및 실행까지 한 사이클을 완성한다.

```
[Windows + VSCode]  ──push──▶  [GitHub]  ──pull──▶  [VMware Rocky Linux]
     편집 & 커밋                                       컴파일 & 실행
```

---

## 만들 파일

### 1. `hello.c` — C 프로그램
```c
#include <stdio.h>

int main(void) {
    printf("Hello\n");
    return 0;
}
```

### 2. `hello.sh` — 셸 스크립트
```bash
#!/bin/bash
echo "Hello"
```

---

## 작업 단계

### 1단계 — VSCode (Windows)
프로젝트 폴더에 위 두 파일을 만든 뒤, VSCode 터미널에서:

```bash
git add hello.c hello.sh
git commit -m "Hello 테스트 파일 추가"
git push
```

### 2단계 — VMware Rocky Linux 터미널
```bash
cd ~/projects/SystemProgrammingPBL
git pull
```

### 3단계 — C 파일 컴파일 & 실행
```bash
gcc hello.c -o hello
./hello
```
**예상 출력:** `Hello`

### 4단계 — 셸 스크립트 실행
```bash
chmod +x hello.sh    # 최초 1회만 필요
./hello.sh
```
**예상 출력:** `Hello`

---

## 문제 해결

| 문제 | 해결 방법 |
|---|---|
| `gcc: command not found` | `sudo dnf install gcc -y` |
| `./hello.sh`에서 `Permission denied` | `chmod +x hello.sh` |
| `git pull`에서 merge conflict 발생 | Windows와 Linux에서 같은 파일을 동시에 편집하지 말 것 (push 안 한 상태로) |
| push 시 비밀번호 요구 | GitHub Settings → Developer settings에서 **Personal Access Token (PAT)** 발급해 사용 |

---

## 개념 정리

- `echo`는 **셸 명령어** — `.sh` 파일 안에서 사용
- `printf`는 **C 함수** — 줄바꿈은 `\n`으로 직접 표시
- `.c` 파일은 실행 전 반드시 **`gcc`로 컴파일**해야 함
- `.sh` 파일은 **실행 권한**(`chmod +x`)이 있어야 실행 가능
- 컴파일 전에 `git pull`로 최신 코드를 먼저 받아야 함

---

## 성공 기준
다음을 모두 만족하면 테스트 완료:
- [ ] GitHub 레포에 `hello.c`와 `hello.sh`가 올라와 있다
- [ ] VMware에서 `git pull` 시 두 파일이 `~/projects/SystemProgrammingPBL`에 들어온다
- [ ] `./hello` 실행 시 `Hello`가 출력된다
- [ ] `./hello.sh` 실행 시 `Hello`가 출력된다

#!/usr/bin/env bash
#
# Размер ячейки и окно больше экрана (задача #4).
#
#   tests/screen/check.sh
#
# Экран в безголовом X известен точно — 800x600, — поэтому «больше экрана»
# здесь не догадка, а заданное условие.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

WORK="${WORK:-$ROOT/build/screen-check}"
DISPLAY_NUM="${DISPLAY_NUM:-:97}"
SCREEN_W=800
SCREEN_H=600
mkdir -p "$WORK"

need() { command -v "$1" > /dev/null || { echo "нужен $1"; exit 2; }; }
need cmake; need gcc; need Xvfb

echo "== сборка =="
cmake -S . -B "$WORK/cmake" -DCMAKE_BUILD_TYPE=Release > "$WORK/cmake.log" 2>&1
cmake --build "$WORK/cmake" --target BearLibTerminal -j"$(nproc)" >> "$WORK/cmake.log" 2>&1
LIB="$(find Output -name 'libBearLibTerminal.so' | head -1)"
[ -n "$LIB" ] || { echo "библиотека не собралась, см. $WORK/cmake.log"; exit 1; }
LIBDIR="$(cd "$(dirname "$LIB")" && pwd)"
gcc tests/screen/scene.c -o "$WORK/screentest" \
    -ITerminal/Include/C -L"$LIBDIR" -lBearLibTerminal -Wl,-rpath,"$LIBDIR"

LOGFILE="$WORK/screen.log"
rm -f "$LOGFILE"

Xvfb "$DISPLAY_NUM" -screen 0 ${SCREEN_W}x${SCREEN_H}x24 > "$WORK/xvfb.log" 2>&1 &
XVFB_PID=$!
trap 'kill $XVFB_PID 2>/dev/null || true' EXIT
sleep 2

DISPLAY="$DISPLAY_NUM" LIBGL_ALWAYS_SOFTWARE=1 \
    "$WORK/screentest" "$LOGFILE" "$SCREEN_W" "$SCREEN_H" > "$WORK/out.log" 2>&1 || {
        echo "проба не отработала:"; cat "$WORK/out.log"; exit 1; }

cat "$WORK/out.log"

fail=0
check() { grep -qF "$1" "$WORK/out.log" && echo "  ок    $2" || { echo "  ПЛОХО $2"; fail=1; }; }

echo "== проверки =="
check "экран: ${SCREEN_W}x${SCREEN_H}"  "библиотека знает размер экрана"
check "ячейка: 32x32"                   "cellsize соблюдён, а не подогнан под ширину глифа"
check "клеток: 200x60"                  "размер сетки не урезан молча"
check "влезает: да"                     "расчёт по экрану даёт влезающее окно"

if grep -qi "larger than the screen" "$LOGFILE"; then
    echo "  ок    в журнале сказано, что окно больше экрана"
else
    echo "  ПЛОХО про окно больше экрана в журнале ни слова"; fail=1
fi

# Обратная проверка: на влезающем окне жаловаться не на что. Проверка, которая
# ругается всегда, ничем не лучше молчания.
if [ "$(grep -ci "larger than the screen" "$LOGFILE")" = "2" ]; then
    echo "  ок    предупреждение ровно там, где надо, и не на влезающем окне"
else
    echo "  ПЛОХО предупреждений $(grep -ci 'larger than the screen' "$LOGFILE"), ожидалось 2"
    fail=1
fi

[ "$fail" = "0" ] || { echo; echo "журнал:"; cat "$LOGFILE"; exit 1; }
echo "поведение определённое"

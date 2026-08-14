#!/usr/bin/env bash
#
# Паскалевская программа не должна падать на пустом месте (задача #9).
#
#   tests/pascal/check.sh
#
# Проба нарочно не выставляет маску исключений сама: если библиотека этого не
# делает, программа падает с EInvalidOp на первом кадре. Проверка ловит именно
# это.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

WORK="${WORK:-$ROOT/build/pascal-check}"
DISPLAY_NUM="${DISPLAY_NUM:-:96}"
mkdir -p "$WORK"

if ! command -v fpc > /dev/null; then
    echo "== Паскаль пропущен: нет fpc =="
    echo "   (это не успех, это непроверенная привязка)"
    exit 0
fi
command -v Xvfb > /dev/null || { echo "нужен Xvfb"; exit 2; }

echo "== сборка библиотеки =="
cmake -S . -B "$WORK/cmake" -DCMAKE_BUILD_TYPE=Release > "$WORK/cmake.log" 2>&1
cmake --build "$WORK/cmake" --target BearLibTerminal -j"$(nproc)" >> "$WORK/cmake.log" 2>&1
LIB="$(find Output -name 'libBearLibTerminal.so' | head -1)"
[ -n "$LIB" ] || { echo "библиотека не собралась, см. $WORK/cmake.log"; exit 1; }
LIBDIR="$(cd "$(dirname "$LIB")" && pwd)"

echo "== сборка пробы =="
mkdir -p "$WORK/units"
fpc -Mobjfpc -Sh -FuTerminal/Include/Pascal -FU"$WORK/units" \
    -k-L"$LIBDIR" -k-rpath -k"$LIBDIR" \
    -o"$WORK/scene" tests/pascal/scene.pas > "$WORK/fpc.log" 2>&1 \
    || { echo "проба не собралась:"; cat "$WORK/fpc.log"; exit 1; }

LOGFILE="$WORK/pascal.log"
rm -f "$LOGFILE"

Xvfb "$DISPLAY_NUM" -screen 0 800x600x24 > "$WORK/xvfb.log" 2>&1 &
XVFB_PID=$!
trap 'kill $XVFB_PID 2>/dev/null || true' EXIT
sleep 2

set +e
DISPLAY="$DISPLAY_NUM" LIBGL_ALWAYS_SOFTWARE=1 "$WORK/scene" "$LOGFILE" > "$WORK/out.log" 2>&1
rc=$?
set -e

fail=0
if [ "$rc" != "0" ]; then
    echo "  ПЛОХО проба завершилась с кодом $rc"
    grep -q "EInvalidOp" "$WORK/out.log" && \
        echo "        это EInvalidOp — маска исключений сопроцессора не выставлена"
    cat "$WORK/out.log"
    fail=1
else
    echo "  ок    программа не упала на первом кадре"
fi

grep -q "дожил до конца" "$WORK/out.log" \
    && echo "  ок    отработала до конца" \
    || { echo "  ПЛОХО до конца не дошла"; fail=1; }

grep -q "паскалевская проба дошла до кадра" "$LOGFILE" 2>/dev/null \
    && echo "  ок    запись в журнал из Паскаля работает" \
    || { echo "  ПЛОХО запись в журнал из Паскаля не дошла"; fail=1; }

[ "$fail" = "0" ] || exit 1
echo "привязка для Паскаля в порядке"

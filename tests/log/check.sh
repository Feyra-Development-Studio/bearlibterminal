#!/usr/bin/env bash
#
# Проверка записи в журнал из приложения (задача #3).
#
#   tests/log/check.sh
#
# Окно здесь всё-таки нужно: настройки применяются только после
# terminal_open — до него terminal_set возвращает -1, — поэтому без окна
# нельзя ни задать файл журнала, ни поднять уровень.
#
# Нужны: cmake, gcc, Xvfb, x11-utils.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

WORK="${WORK:-$ROOT/build/log-check}"
DISPLAY_NUM="${DISPLAY_NUM:-:98}"
mkdir -p "$WORK"

need() { command -v "$1" > /dev/null || { echo "нужен $1"; exit 2; }; }
need cmake; need gcc; need Xvfb

echo "== сборка библиотеки =="
cmake -S . -B "$WORK/cmake" -DCMAKE_BUILD_TYPE=Release > "$WORK/cmake.log" 2>&1
cmake --build "$WORK/cmake" --target BearLibTerminal -j"$(nproc)" >> "$WORK/cmake.log" 2>&1
LIB="$(find Output -name 'libBearLibTerminal.so' | head -1)"
[ -n "$LIB" ] || { echo "библиотека не собралась, см. $WORK/cmake.log"; exit 1; }
LIBDIR="$(cd "$(dirname "$LIB")" && pwd)"

gcc tests/log/scene.c -o "$WORK/logtest" \
    -ITerminal/Include/C -L"$LIBDIR" -lBearLibTerminal -Wl,-rpath,"$LIBDIR"

LOGFILE="$WORK/app.log"
rm -f "$LOGFILE"

Xvfb "$DISPLAY_NUM" -screen 0 640x480x24 > "$WORK/xvfb.log" 2>&1 &
XVFB_PID=$!
trap 'kill $XVFB_PID 2>/dev/null || true' EXIT
sleep 2

DISPLAY="$DISPLAY_NUM" LIBGL_ALWAYS_SOFTWARE=1 "$WORK/logtest" "$LOGFILE" \
    > "$WORK/stdout.log" 2> "$WORK/stderr.log" || {
        echo "проба не отработала:"; cat "$WORK/stdout.log" "$WORK/stderr.log"; exit 1; }

fail=0
expect()   { grep -qF "$1" "$LOGFILE" && echo "  ок    $2" || { echo "  ПЛОХО $2"; fail=1; }; }
unexpect() { grep -qF "$1" "$LOGFILE" && { echo "  ПЛОХО $2"; fail=1; } || echo "  ок    $2"; }

echo "== содержимое журнала =="
expect   "[app] сообщение приложения"  "сообщение приложения дошло до журнала"
expect   "[app]"                       "источник помечен: видно, где приложение"
expect   "широкая строка"              "широкие строки не теряются"
unexpect "[app] это не должно попасть" "уровень фильтрует лишнее"
expect   "причина падения"             "запись до закрытия не стёрта записью после"
expect   "после закрытия"              "запись после terminal_close попадает в журнал"

# Ошибка самой библиотеки должна лежать в том же файле: иначе смысла в общем
# журнале нет — придётся смотреть в два места.
expect   "Failed to parse"             "сообщения библиотеки в том же журнале"

if [ "$fail" != "0" ]; then
    echo; echo "журнал целиком:"; cat "$LOGFILE"; exit 1
fi
echo "журнал в порядке"

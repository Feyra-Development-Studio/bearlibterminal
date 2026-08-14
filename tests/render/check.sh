#!/usr/bin/env bash
#
# Сверка отрисовки: рисуем неподвижную сцену в безголовом X и сравниваем
# снимок с эталоном.
#
#   tests/render/check.sh            сверить с эталоном
#   tests/render/check.sh --update   перезаписать эталон
#
# Зачем. Отрисовка в библиотеке общая для всех платформ. Перевод вывода на
# GLES2 ради Android (задача #1) меняет тот же код, которым рисуют Linux и
# Windows, поэтому испортить картинку на них — самый вероятный способ
# провалить задачу. Сверять на глаз бесполезно: разница в один пиксель на
# краю глифа глазом не видна, а это уже другой шрифт.
#
# Эталон перезаписывается только руками и только осознанно. Если правка
# меняет картинку намеренно, вместе с новым эталоном в коммите должно быть
# сказано, что именно изменилось и почему.
#
# Нужны: cmake, gcc, Xvfb, imagemagick, x11-utils.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

WORK="${WORK:-$ROOT/build/render-check}"
REFERENCE="tests/render/reference-linux64.png"
DISPLAY_NUM="${DISPLAY_NUM:-:99}"
UPDATE=0
[ "${1:-}" = "--update" ] && UPDATE=1

mkdir -p "$WORK"

need() { command -v "$1" > /dev/null || { echo "нужен $1"; exit 2; }; }
need cmake; need gcc; need Xvfb; need import; need compare; need xwininfo

echo "== сборка библиотеки =="
cmake -S . -B "$WORK/cmake" -DCMAKE_BUILD_TYPE=Release > "$WORK/cmake.log" 2>&1
cmake --build "$WORK/cmake" --target BearLibTerminal -j"$(nproc)" >> "$WORK/cmake.log" 2>&1
LIB="$(find Output -name 'libBearLibTerminal.so' | head -1)"
[ -n "$LIB" ] || { echo "библиотека не собралась, см. $WORK/cmake.log"; exit 1; }
LIBDIR="$(cd "$(dirname "$LIB")" && pwd)"

echo "== сборка сцены =="
gcc tests/render/scene.c -o "$WORK/scene" \
    -ITerminal/Include/C -L"$LIBDIR" -lBearLibTerminal -Wl,-rpath,"$LIBDIR"

echo "== снимок =="
Xvfb "$DISPLAY_NUM" -screen 0 800x600x24 > "$WORK/xvfb.log" 2>&1 &
XVFB_PID=$!
# Прибираем за собой в любом случае, включая падение посередине.
trap 'kill $XVFB_PID 2>/dev/null || true' EXIT

for _ in $(seq 1 20); do
    DISPLAY="$DISPLAY_NUM" xwininfo -root > /dev/null 2>&1 && break
    sleep 0.5
done

# Программный вывод: на бегунке CI видеокарты нет, а картинка должна
# получаться та же самая. llvmpipe считает без плавающей точки в спорных
# местах, поэтому снимок устойчив от запуска к запуску.
DISPLAY="$DISPLAY_NUM" LIBGL_ALWAYS_SOFTWARE=1 "$WORK/scene" > "$WORK/scene.log" 2>&1 &
SCENE_PID=$!

WID=""
for _ in $(seq 1 30); do
    sleep 0.5
    WID="$(DISPLAY="$DISPLAY_NUM" xwininfo -root -children 2>/dev/null \
           | grep -oE '0x[0-9a-f]+ "blt-render-check"' | cut -d' ' -f1 || true)"
    [ -n "$WID" ] && break
done
[ -n "$WID" ] || { echo "окно не появилось, см. $WORK/scene.log"; cat "$WORK/scene.log"; exit 1; }

# Даём отрисоваться первому кадру: окно появляется раньше, чем в нём что-то есть.
sleep 2
DISPLAY="$DISPLAY_NUM" import -window "$WID" "$WORK/shot.png"
kill "$SCENE_PID" 2>/dev/null || true

if [ "$UPDATE" = "1" ]; then
    cp "$WORK/shot.png" "$REFERENCE"
    echo "эталон перезаписан: $REFERENCE"
    identify "$REFERENCE"
    exit 0
fi

if [ ! -f "$REFERENCE" ]; then
    echo "эталона нет. Создайте его: tests/render/check.sh --update"
    exit 1
fi

echo "== сравнение =="
DIFF="$(compare -metric AE "$WORK/shot.png" "$REFERENCE" "$WORK/diff.png" 2>&1 || true)"
if [ "$DIFF" = "0" ]; then
    echo "картинка не изменилась"
    exit 0
fi

echo "картинка изменилась: различается пикселей — $DIFF"
echo "  снимок:   $WORK/shot.png"
echo "  эталон:   $REFERENCE"
echo "  различия: $WORK/diff.png"
echo
echo "Если это намеренно, обновите эталон (--update) и объясните в коммите,"
echo "что именно изменилось."
exit 1

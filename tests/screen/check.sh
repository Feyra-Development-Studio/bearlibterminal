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

# Несколько размеров экрана, а не один: при одном размере работающий расчёт
# неотличим от совпадения. Узкий и высокий здесь нарочно — на телефоне экран
# именно такой, и деление по разным сторонам должно давать разное.
SCREENS="${SCREENS:-800x600 1024x768 1920x1080 480x800}"
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

fail=0

run_one() {
    local screen="$1" runner="$2" exe="$3" label="$4"
    local w="${screen%x*}" h="${screen#*x}"
    local out="$WORK/out-$label-$screen.log"
    local log="$WORK/log-$label-$screen.log"
    rm -f "$log"

    Xvfb "$DISPLAY_NUM" -screen 0 "${w}x${h}x24" > "$WORK/xvfb.log" 2>&1 &
    local xvfb_pid=$!
    sleep 2

    if ! DISPLAY="$DISPLAY_NUM" LIBGL_ALWAYS_SOFTWARE=1 WINEDEBUG=-all \
         $runner "$exe" "$log" "$w" "$h" > "$out" 2>&1; then
        echo "  ПЛОХО [$label $screen] проба не отработала"; cat "$out"; fail=1
        kill $xvfb_pid 2>/dev/null || true; return
    fi
    kill $xvfb_pid 2>/dev/null || true
    sleep 1

    # Windows-сборка пишет строки с возвратом каретки. Без этого сравнения
    # ломаются на невидимом символе, и проверка ругается на исправный код.
    sed -i 's/\r$//' "$out" "$log"

    local got_cells got_px cell fits
    got_cells="$(grep -oP '(?<=^клеток: )[0-9]+x[0-9]+' "$out" || true)"
    got_px="$(grep -oP '(?<= = )[0-9]+x[0-9]+(?= точек)' "$out" || true)"
    cell="$(grep -oP '(?<=^ячейка после урезания: )[0-9]+x[0-9]+' "$out" || true)"
    fits="$(grep -oP '(?<=^влезает: ).*' "$out" || true)"

    # Ожидаемое считается здесь же, независимо от библиотеки: 32-точечная
    # ячейка, значит клеток ровно столько, сколько их укладывается в экран.
    local want="$(( w / 32 ))x$(( h / 32 ))"

    if [ "$got_cells" = "$want" ]; then
        echo "  ок    [$label $screen] сетка урезана до $got_cells ($got_px точек)"
    else
        echo "  ПЛОХО [$label $screen] сетка $got_cells, ожидалась $want"; fail=1
    fi
    [ "$cell" = "32x32" ] || { echo "  ПЛОХО [$label $screen] ячейка $cell вместо 32x32"; fail=1; }
    [ "$fits" = "да" ]    || { echo "  ПЛОХО [$label $screen] окно не влезло в экран"; fail=1; }

    if grep -qP '^влезающее просили (\d+x\d+), получили \1$' "$out"; then
        echo "  ок    [$label $screen] влезающий размер не тронут"
    else
        echo "  ПЛОХО [$label $screen] влезающий размер изменён: $(grep '^влезающее' "$out")"; fail=1
    fi

    # Размер окна пишется при создании и при изменении — не на каждый вызов
    # настроек. Иначе важное тонет в подробностях.
    local sizes noise
    sizes="$(grep -c "Window size is" "$log" || true)"
    noise="$(grep -c "Trying to set" "$log" || true)"
    if [ "$sizes" -gt 0 ] && [ "$sizes" -lt "$noise" ]; then
        echo "  ок    [$label $screen] размер в журнале $sizes раз при $noise настройках"
    else
        echo "  ПЛОХО [$label $screen] размер в журнале $sizes раз при $noise настройках"; fail=1
    fi
}

echo "== Linux, оконный и полноэкранный =="
for screen in $SCREENS; do
    run_one "$screen" "" "$WORK/screentest" linux
done

# Та самая платформа, где дефект и наблюдали. Без этой проверки задача
# считалась бы сделанной, не будучи проверенной там, где важно.
if command -v x86_64-w64-mingw32-gcc > /dev/null && command -v wine > /dev/null; then
    echo "== Windows через MinGW и Wine =="
    cmake -S . -B "$WORK/cmake-win" -DCMAKE_TOOLCHAIN_FILE="$ROOT/tests/screen/mingw.cmake" \
          -DCMAKE_BUILD_TYPE=Release >> "$WORK/cmake.log" 2>&1
    cmake --build "$WORK/cmake-win" --target BearLibTerminal -j"$(nproc)" >> "$WORK/cmake.log" 2>&1
    WINLIB="$(find Output -name 'BearLibTerminal.dll' | head -1)"
    if [ -n "$WINLIB" ]; then
        mkdir -p "$WORK/win"
        cp "$WINLIB" "$WORK/win/"
        x86_64-w64-mingw32-gcc tests/screen/scene.c -o "$WORK/win/screentest.exe" \
            -ITerminal/Include/C -L"$(dirname "$WINLIB")" -lBearLibTerminal
        for screen in $SCREENS; do
            run_one "$screen" wine "$WORK/win/screentest.exe" windows
        done
    else
        echo "  ПЛОХО библиотека под Windows не собралась"; fail=1
    fi
else
    echo "== Windows пропущен: нет x86_64-w64-mingw32-gcc или wine =="
    echo "   (это не успех, это непроверенная платформа)"
fi

[ "$fail" = "0" ] || exit 1
echo "поведение определённое на всех проверенных размерах и платформах"

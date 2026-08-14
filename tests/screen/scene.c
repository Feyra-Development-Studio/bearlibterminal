/* Проба для задачи #4: размер ячейки и окно больше экрана.
 *
 * Доводы: путь к журналу, ширина и высота экрана (их задаёт check.sh, поднимая
 * Xvfb известного размера).
 */

#include "BearLibTerminal.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	char setting[256];
	int cols, rows, want_w, want_h;

	if (argc < 4)
	{
		printf("нужны: файл журнала, ширина и высота экрана\n");
		return 2;
	}

	if (!terminal_open())
	{
		printf("terminal_open не отработал\n");
		return 1;
	}

	snprintf(setting, sizeof(setting), "log: file=%s, level=info", argv[1]);
	terminal_set(setting);

	printf("экран: %dx%d\n",
		terminal_state(TK_SCREEN_WIDTH), terminal_state(TK_SCREEN_HEIGHT));

	/* Жалоба с форума: ширина ячейки якобы остаётся пропорциональной ширине
	   исходного глифа. В нынешнем виде библиотека задаёт обе стороны. */
	terminal_set("window: cellsize=32x32");
	printf("ячейка: %dx%d\n",
		terminal_state(TK_CELL_WIDTH), terminal_state(TK_CELL_HEIGHT));

	/* Заведомо больше экрана. Сетка не должна урезаться молча: размер —
	   это обещание приложению, что по этим координатам можно печатать. */
	terminal_set("window: size=200x60");
	printf("клеток: %dx%d\n", terminal_state(TK_WIDTH), terminal_state(TK_HEIGHT));

	/* А теперь то, ради чего наружу выведен размер экрана: приложение само
	   считает, сколько клеток влезает, и получает окно по размеру экрана. */
	cols = terminal_state(TK_SCREEN_WIDTH) / terminal_state(TK_CELL_WIDTH);
	rows = terminal_state(TK_SCREEN_HEIGHT) / terminal_state(TK_CELL_HEIGHT);
	snprintf(setting, sizeof(setting), "window: size=%dx%d", cols, rows);
	terminal_set(setting);

	want_w = terminal_state(TK_WIDTH) * terminal_state(TK_CELL_WIDTH);
	want_h = terminal_state(TK_HEIGHT) * terminal_state(TK_CELL_HEIGHT);
	printf("подобрано: %dx%d клеток = %dx%d точек\n",
		terminal_state(TK_WIDTH), terminal_state(TK_HEIGHT), want_w, want_h);
	printf("влезает: %s\n",
		(want_w <= terminal_state(TK_SCREEN_WIDTH) &&
		 want_h <= terminal_state(TK_SCREEN_HEIGHT))? "да": "нет");

	terminal_close();
	return 0;
}

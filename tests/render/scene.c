/* Неподвижная сцена для сверки отрисовки.
 *
 * Нужна затем, чтобы перевод вывода с фиксированного конвейера OpenGL 1.x на
 * GLES2 можно было проверить машинно, а не на глаз. Отрисовка у библиотеки
 * общая для всех платформ, поэтому починка Android — это самый быстрый способ
 * незаметно испортить картинку на Linux и Windows.
 *
 * Сцена подобрана так, чтобы задеть всё, что переводится на шейдеры:
 * глифы, цвет переднего плана и фона, подложку, наложение полупрозрачного,
 * составные символы вне ASCII. Ничего движущегося и ничего случайного —
 * снимок обязан совпадать побайтово от запуска к запуску.
 */

#include "BearLibTerminal.h"

int main(void)
{
	if (!terminal_open())
		return 1;

	/* Имя окна ищет check.sh, чтобы снимать именно его, а не весь экран. */
	terminal_set("window: size=40x12, title='blt-render-check', resizeable=false;"
	             " font: default; input: filter=[keyboard]");

	terminal_bkcolor(color_from_name("black"));
	terminal_clear();

	/* Обычный текст. */
	terminal_color(color_from_name("white"));
	terminal_print(1, 1, "BearLibTerminal render check");

	/* Глифы карты: стены, пол, герой — то, чем рисуется игра. */
	terminal_color(color_from_name("orange"));
	terminal_print(1, 3, "########");
	terminal_color(color_from_name("dark gray"));
	terminal_print(10, 3, "........");
	terminal_color(color_from_name("yellow"));
	terminal_print(19, 3, "@");

	/* Символы вне ASCII: отдельная страница глифов в текстуре. */
	terminal_color(color_from_name("cyan"));
	terminal_print(1, 5, "этаж 1/10  x=21 y=9");

	/* Цвет фона отдельной клетки. */
	terminal_bkcolor(color_from_name("dark blue"));
	terminal_print(1, 7, " подложка под текстом ");
	terminal_bkcolor(color_from_name("black"));

	/* Полупрозрачное поверх нарисованного: смешивание цветов. */
	terminal_color(0x80FF0000);
	terminal_print(1, 9, "полупрозрачный слой");

	terminal_refresh();

	/* Держим окно, пока check.sh снимает. Ввод не читаем: клавиатуры в
	   безголовом X нет, а terminal_read ждал бы её вечно. */
	terminal_delay(20000);
	terminal_close();
	return 0;
}

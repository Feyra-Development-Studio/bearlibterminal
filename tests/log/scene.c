/* Проба записи в журнал из приложения (задача #3).
 *
 * Путь к файлу журнала передаётся первым доводом: складывать его рядом с
 * исходниками нельзя, проверка должна писать только в свой рабочий каталог.
 */

#include "BearLibTerminal.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
	char setting[512];

	if (argc < 2)
	{
		printf("нужен путь к файлу журнала\n");
		return 2;
	}

	if (!terminal_open())
	{
		printf("terminal_open не отработал\n");
		return 1;
	}

	/* Настройки применяются только после terminal_open: до него
	   terminal_set возвращает -1 и молча ничего не делает. */
	snprintf(setting, sizeof(setting), "log: file=%s, level=info", argv[1]);
	if (terminal_set(setting) != 1)
	{
		printf("не удалось настроить журнал\n");
		return 1;
	}

	terminal_log(TK_LOG_INFO, "сообщение приложения");
	terminal_wlog(TK_LOG_INFO, L"широкая строка: этаж 1/10 x=21 y=9");
	terminal_log(TK_LOG_DEBUG, "это не должно попасть при level=info");
	terminal_log(TK_LOG_INFO, NULL);   /* пустой указатель не должен ронять */

	/* Ошибка самой библиотеки: лишний пробел в имени набора. Должна лечь в
	   тот же журнал, что и сообщения приложения. */
	terminal_set("bad  font: /nonexistent.ttf, size=12");

	terminal_log(TK_LOG_ERROR, "причина падения");

	terminal_close();

	/* Запись после закрытия раньше обрезала файл и уничтожала всё, что было
	   записано за сеанс, — то есть ровно те сведения, ради которых журнал и
	   ведётся. Обе строки обязаны остаться. */
	terminal_log(TK_LOG_ERROR, "после закрытия");

	printf("проба отработала\n");
	return 0;
}

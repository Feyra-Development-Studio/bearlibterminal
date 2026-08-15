/*
* BearLibTerminal
* Copyright (C) 2013-2017 Cfyz
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef BEARLIBTERMINAL_ANDROIDWINDOW_HPP
#define BEARLIBTERMINAL_ANDROIDWINDOW_HPP

#ifdef __ANDROID__

#include "Window.hpp"
#include "Size.hpp"
#include <cstdint>
#include <EGL/egl.h>
#include <android/native_window.h>

namespace BearLibTerminal
{
	/*
	 * Окно на Android.
	 *
	 * Устроено принципиально иначе, чем настольные: окна здесь не создают.
	 * Поверхность выдаёт система, когда сочтёт нужным, и отбирает, когда
	 * приложение сворачивают, — вместе с контекстом GL и всем, что в нём
	 * лежало. Поэтому SetClientSize, SetTitle, SetResizeable и прочее из
	 * настольного набора здесь ничего не делают: размер и заголовок задаёт не
	 * приложение.
	 *
	 * Поверхность передаётся снаружи, из деятельности приложения (activity),
	 * через AttachSurface. До этого рисовать некуда, и AcquireRC об этом
	 * честно сообщает, а не притворяется, что всё хорошо.
	 */
	class AndroidWindow: public Window
	{
	public:
		AndroidWindow(EventHandler handler);
		~AndroidWindow();

		void SetTitle(const std::wstring& title);
		void SetIcon(const std::wstring& filename);
		void SetClientSize(const Size& size);
		void Show();
		void Hide();
		void AcquireRC();
		void ReleaseRC();
		void SwapBuffers();
		void SetVSync(bool enabled);
		int PumpEvents();
		void SetResizeable(bool resizeable);
		Size GetActualSize();
		Size GetScreenSize();
		std::wstring GetClipboard();
		void SetFullscreen(bool fullscreen);
		void SetCursorVisibility(bool visible);

		// Вызывается из деятельности приложения при появлении и пропаже
		// поверхности. Второе — не ошибка, а обычный ход событий: сворачивание.
		void AttachSurface(ANativeWindow* window);
		void DetachSurface();

		/* Ввод приходит снаружи, из деятельности приложения: своей очереди
		   событий у окна на Android нет.

		   Касание и мышь намеренно не различаются. Мышь в библиотеке есть
		   давно и работает на всех настольных платформах; касание приходит
		   теми же TK_MOUSE_*, и разбирается тем же кодом игры. Отдельного
		   способа ввода для Android не заводится — иначе игре пришлось бы
		   знать, на чём её запустили. */
		/* is_touch различает палец и мышь.

		   Наружу они дают одни и те же события, но посылаются те в разное
		   время. У мыши нажатие означает нажатие: ему всегда предшествует
		   движение, и протаскивание с зажатой кнопкой — обычное дело.
		   Касание — это начало жеста, и чем он окажется, известно только
		   когда палец оторвали. */
		void HandlePointer(int action, int x, int y, bool is_touch);
		void HandleKey(int keycode, bool pressed, int unicode);

		// Порог, дальше которого движение пальца перестаёт быть щелчком.
		// Задаётся приложением: он зависит от плотности точек экрана, о
		// которой библиотека не знает, а Java знает.
		void SetTouchSlop(int pixels);

		// Значения action — те же, что у AMotionEvent, чтобы деятельности
		// приложения не приходилось их переводить.
		static const int kPointerDown = 0;
		static const int kPointerUp = 1;
		static const int kPointerMove = 2;

	private:
		void EmitClick(bool pressed, int x, int y);
		bool CreateContext();
		void DestroyContext();

		ANativeWindow* m_native_window;
		uint64_t m_last_pointer_press;
		int m_consecutive_clicks;
		bool m_touch_active;
		bool m_touch_dragged;
		int m_touch_start_x;
		int m_touch_start_y;
		int m_touch_slop;
		EGLDisplay m_display;
		EGLSurface m_surface;
		EGLContext m_context;
		Size m_size;
	};
}

#endif // __ANDROID__

#endif // BEARLIBTERMINAL_ANDROIDWINDOW_HPP

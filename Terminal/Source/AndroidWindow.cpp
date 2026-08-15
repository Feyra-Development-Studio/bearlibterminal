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

#ifdef __ANDROID__

#include "AndroidWindow.hpp"
#include "OpenGL.hpp"
#include "Log.hpp"
// Вывод Size в поток объявлен здесь, а не в Size.hpp. На настольных
// платформах это не всплывало: там его подтягивали соседние заголовки.
#include "Geometry.hpp"
#include "Utility.hpp"
// Константы TK_* объявлены в открытом заголовке; платформенные исходники
// подключают его сами — так же делает X11Window через Terminal.hpp.
#include "BearLibTerminal.h"

namespace BearLibTerminal
{
	AndroidWindow::AndroidWindow(EventHandler handler):
		Window(handler),
		m_native_window(nullptr),
		m_display(EGL_NO_DISPLAY),
		m_surface(EGL_NO_SURFACE),
		m_context(EGL_NO_CONTEXT),
		m_last_pointer_press(0),
		m_consecutive_clicks(0)
	{ }

	AndroidWindow::~AndroidWindow()
	{
		DestroyContext();
	}

	void AndroidWindow::AttachSurface(ANativeWindow* window)
	{
		if (m_native_window == window)
			return;

		DestroyContext();
		m_native_window = window;

		if (m_native_window != nullptr)
			CreateContext();
	}

	void AndroidWindow::DetachSurface()
	{
		AttachSurface(nullptr);
	}

	bool AndroidWindow::CreateContext()
	{
		m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (m_display == EGL_NO_DISPLAY)
		{
			LOG(Fatal, L"EGL: no display");
			return false;
		}

		if (!eglInitialize(m_display, nullptr, nullptr))
		{
			LOG(Fatal, L"EGL: initialization failed");
			return false;
		}

		// Просим GLES3, как решено для порта. Восьми бит на составляющую и
		// буфера глубины не требуется вовсе: терминал рисует плоскую сетку.
		const EGLint config_attributes[] =
		{
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_ALPHA_SIZE, 8,
			EGL_NONE
		};

		EGLConfig config;
		EGLint config_count = 0;
		if (!eglChooseConfig(m_display, config_attributes, &config, 1, &config_count) || config_count < 1)
		{
			LOG(Fatal, L"EGL: no suitable configuration for GLES3");
			return false;
		}

		// Формат буфера окна обязан совпадать с выбранной конфигурацией,
		// иначе eglCreateWindowSurface откажет — и откажет невнятно.
		EGLint format = 0;
		eglGetConfigAttrib(m_display, config, EGL_NATIVE_VISUAL_ID, &format);
		ANativeWindow_setBuffersGeometry(m_native_window, 0, 0, format);

		m_surface = eglCreateWindowSurface(m_display, config, m_native_window, nullptr);
		if (m_surface == EGL_NO_SURFACE)
		{
			LOG(Fatal, L"EGL: window surface was not created");
			return false;
		}

		const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
		m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, context_attributes);
		if (m_context == EGL_NO_CONTEXT)
		{
			LOG(Fatal, L"EGL: context was not created");
			return false;
		}

		if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context))
		{
			LOG(Fatal, L"EGL: context was not made current");
			return false;
		}

		EGLint width = 0, height = 0;
		eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width);
		eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height);
		m_size = Size(width, height);
		LOG(Info, L"EGL: surface is " << m_size << L" pixels");

		ProbeOpenGL();
		return true;
	}

	void AndroidWindow::DestroyContext()
	{
		if (m_display == EGL_NO_DISPLAY)
			return;

		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (m_context != EGL_NO_CONTEXT)
			eglDestroyContext(m_display, m_context);
		if (m_surface != EGL_NO_SURFACE)
			eglDestroySurface(m_display, m_surface);

		eglTerminate(m_display);

		m_display = EGL_NO_DISPLAY;
		m_surface = EGL_NO_SURFACE;
		m_context = EGL_NO_CONTEXT;
	}

	void AndroidWindow::AcquireRC()
	{
		if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE)
			return;
		eglMakeCurrent(m_display, m_surface, m_surface, m_context);
	}

	void AndroidWindow::ReleaseRC()
	{
		if (m_display == EGL_NO_DISPLAY)
			return;
		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}

	void AndroidWindow::SwapBuffers()
	{
		if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE)
			return;
		eglSwapBuffers(m_display, m_surface);
	}

	void AndroidWindow::SetVSync(bool enabled)
	{
		if (m_display == EGL_NO_DISPLAY)
			return;
		eglSwapInterval(m_display, enabled? 1: 0);
	}

	Size AndroidWindow::GetActualSize()
	{
		return m_size;
	}

	Size AndroidWindow::GetScreenSize()
	{
		// Окно здесь и есть экран, поэтому отдельного размера экрана не
		// существует. Это же значение получит и приложение через
		// TK_SCREEN_WIDTH/TK_SCREEN_HEIGHT, из которых оно считает сетку.
		return m_size;
	}

	void AndroidWindow::HandlePointer(int action, int x, int y)
	{
		// Положение сообщается всегда, включая нажатие: на сенсорном экране
		// указателя нет, и до касания библиотека не знает, где палец. Если
		// послать только нажатие, клетка окажется прежней — то есть игрок
		// пойдёт не туда, куда ткнул.
		Event move(TK_MOUSE_MOVE);
		move[TK_MOUSE_PIXEL_X] = x;
		move[TK_MOUSE_PIXEL_Y] = y;
		m_event_handler(std::move(move));

		if (action == kPointerMove)
			return;

		bool pressed = (action == kPointerDown);

		if (pressed)
		{
			// Счёт подряд идущих нажатий — как на настольных платформах,
			// с тем же порогом в четверть секунды, чтобы двойное касание и
			// двойной щелчок означали для игры одно и то же.
			uint64_t now = gettime();
			uint64_t delta = now - m_last_pointer_press;
			m_last_pointer_press = now;
			m_consecutive_clicks = (delta < 250000)? m_consecutive_clicks + 1: 1;
		}

		Event event(TK_MOUSE_LEFT | (pressed? 0: TK_KEY_RELEASED));
		event[TK_MOUSE_LEFT] = pressed? 1: 0;
		event[TK_MOUSE_CLICKS] = pressed? m_consecutive_clicks: 0;
		m_event_handler(std::move(event));
	}

	void AndroidWindow::HandleKey(int keycode, bool pressed, int unicode)
	{
		if (keycode == 0)
			return;

		Event event(keycode | (pressed? 0: TK_KEY_RELEASED));
		event[keycode] = pressed? 1: 0;

		// Печатный знак приходит от деятельности приложения уже разобранным:
		// раскладки, составные знаки и предсказание ввода живут в Java, и
		// повторять эту работу здесь было бы и глупо, и хуже.
		if (pressed && unicode > 0)
			event[TK_WCHAR] = unicode;

		m_event_handler(std::move(event));
	}

	int AndroidWindow::PumpEvents()
	{
		// События приходят из деятельности приложения, а не выбираются
		// отсюда: своей очереди у окна нет.
		return 0;
	}

	// Ниже — то, чего на Android не бывает. Пустые тела здесь честнее
	// заглушек с выдуманным поведением: размер, заголовок и курсор задаёт
	// система, а не приложение.
	void AndroidWindow::SetTitle(const std::wstring&) { }
	void AndroidWindow::SetIcon(const std::wstring&) { }
	void AndroidWindow::SetClientSize(const Size&) { }
	void AndroidWindow::Show() { }
	void AndroidWindow::Hide() { }
	void AndroidWindow::SetResizeable(bool) { }
	void AndroidWindow::SetFullscreen(bool) { }
	void AndroidWindow::SetCursorVisibility(bool) { }

	std::wstring AndroidWindow::GetClipboard()
	{
		// Буфер обмена на Android живёт в Java, добраться до него из
		// нативного кода без JNI нельзя. Пока не понадобилось.
		return std::wstring();
	}
}

#endif // __ANDROID__

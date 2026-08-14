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

namespace BearLibTerminal
{
	AndroidWindow::AndroidWindow(EventHandler handler):
		Window(handler),
		m_native_window(nullptr),
		m_display(EGL_NO_DISPLAY),
		m_surface(EGL_NO_SURFACE),
		m_context(EGL_NO_CONTEXT)
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
			LOG(Fatal, "EGL: no display");
			return false;
		}

		if (!eglInitialize(m_display, nullptr, nullptr))
		{
			LOG(Fatal, "EGL: initialization failed");
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
			LOG(Fatal, "EGL: no suitable configuration for GLES3");
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
			LOG(Fatal, "EGL: window surface was not created");
			return false;
		}

		const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
		m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, context_attributes);
		if (m_context == EGL_NO_CONTEXT)
		{
			LOG(Fatal, "EGL: context was not created");
			return false;
		}

		if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context))
		{
			LOG(Fatal, "EGL: context was not made current");
			return false;
		}

		EGLint width = 0, height = 0;
		eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width);
		eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height);
		m_size = Size(width, height);
		LOG(Info, "EGL: surface is " << m_size << " pixels");

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

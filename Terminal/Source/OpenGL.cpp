/*
* BearLibTerminal
* Copyright (C) 2013 Cfyz
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

#include "OpenGL.hpp"
#include "Log.hpp"
#include "VertexBatch.hpp"
#include <string>
#include <algorithm>

#if !defined(__ANDROID__)
#  if defined(_WIN32)
#    include <windows.h>
#  else
#    include <GL/glx.h>
#  endif
#endif

namespace BearLibTerminal
{
#if !defined(__ANDROID__)
	PFN_glCreateShader bltCreateShader = nullptr;
	PFN_glShaderSource bltShaderSource = nullptr;
	PFN_glCompileShader bltCompileShader = nullptr;
	PFN_glGetShaderiv bltGetShaderiv = nullptr;
	PFN_glGetShaderInfoLog bltGetShaderInfoLog = nullptr;
	PFN_glDeleteShader bltDeleteShader = nullptr;
	PFN_glCreateProgram bltCreateProgram = nullptr;
	PFN_glAttachShader bltAttachShader = nullptr;
	PFN_glLinkProgram bltLinkProgram = nullptr;
	PFN_glGetProgramiv bltGetProgramiv = nullptr;
	PFN_glGetProgramInfoLog bltGetProgramInfoLog = nullptr;
	PFN_glUseProgram bltUseProgram = nullptr;
	PFN_glGetAttribLocation bltGetAttribLocation = nullptr;
	PFN_glGetUniformLocation bltGetUniformLocation = nullptr;
	PFN_glUniform1i bltUniform1i = nullptr;
	PFN_glUniformMatrix4fv bltUniformMatrix4fv = nullptr;
	PFN_glEnableVertexAttribArray bltEnableVertexAttribArray = nullptr;
	PFN_glDisableVertexAttribArray bltDisableVertexAttribArray = nullptr;
	PFN_glVertexAttribPointer bltVertexAttribPointer = nullptr;
	PFN_glGenBuffers bltGenBuffers = nullptr;
	PFN_glBindBuffer bltBindBuffer = nullptr;
	PFN_glBufferData bltBufferData = nullptr;
	PFN_glDeleteBuffers bltDeleteBuffers = nullptr;

	namespace
	{
		void* GetProc(const char* name)
		{
#if defined(_WIN32)
			// wglGetProcAddress only knows about entry points past 1.1, and
			// on some drivers it returns 1, 2, 3 or -1 instead of null for
			// the ones it does not know. Hence the explicit sifting.
			void* p = (void*)wglGetProcAddress(name);
			intptr_t v = (intptr_t)p;
			if (v == 0 || v == 1 || v == 2 || v == 3 || v == -1)
			{
				HMODULE module = GetModuleHandleA("opengl32.dll");
				p = module? (void*)GetProcAddress(module, name): nullptr;
			}
			return p;
#else
			return (void*)glXGetProcAddress((const GLubyte*)name);
#endif
		}

		bool g_entry_points_loaded = false;
	}

	bool LoadOpenGLEntryPoints()
	{
		if (g_entry_points_loaded)
			return true;

		bool complete = true;
		auto load = [&complete](const char* name) -> void*
		{
			void* p = GetProc(name);
			if (p == nullptr)
			{
				LOG(Error, "OpenGL: entry point " << name << " is missing");
				complete = false;
			}
			return p;
		};

		bltCreateShader = (PFN_glCreateShader)load("glCreateShader");
		bltShaderSource = (PFN_glShaderSource)load("glShaderSource");
		bltCompileShader = (PFN_glCompileShader)load("glCompileShader");
		bltGetShaderiv = (PFN_glGetShaderiv)load("glGetShaderiv");
		bltGetShaderInfoLog = (PFN_glGetShaderInfoLog)load("glGetShaderInfoLog");
		bltDeleteShader = (PFN_glDeleteShader)load("glDeleteShader");
		bltCreateProgram = (PFN_glCreateProgram)load("glCreateProgram");
		bltAttachShader = (PFN_glAttachShader)load("glAttachShader");
		bltLinkProgram = (PFN_glLinkProgram)load("glLinkProgram");
		bltGetProgramiv = (PFN_glGetProgramiv)load("glGetProgramiv");
		bltGetProgramInfoLog = (PFN_glGetProgramInfoLog)load("glGetProgramInfoLog");
		bltUseProgram = (PFN_glUseProgram)load("glUseProgram");
		bltGetAttribLocation = (PFN_glGetAttribLocation)load("glGetAttribLocation");
		bltGetUniformLocation = (PFN_glGetUniformLocation)load("glGetUniformLocation");
		bltUniform1i = (PFN_glUniform1i)load("glUniform1i");
		bltUniformMatrix4fv = (PFN_glUniformMatrix4fv)load("glUniformMatrix4fv");
		bltEnableVertexAttribArray = (PFN_glEnableVertexAttribArray)load("glEnableVertexAttribArray");
		bltDisableVertexAttribArray = (PFN_glDisableVertexAttribArray)load("glDisableVertexAttribArray");
		bltVertexAttribPointer = (PFN_glVertexAttribPointer)load("glVertexAttribPointer");
		bltGenBuffers = (PFN_glGenBuffers)load("glGenBuffers");
		bltBindBuffer = (PFN_glBindBuffer)load("glBindBuffer");
		bltBufferData = (PFN_glBufferData)load("glBufferData");
		bltDeleteBuffers = (PFN_glDeleteBuffers)load("glDeleteBuffers");

		g_entry_points_loaded = complete;
		return complete;
	}
#endif

	int g_max_texture_size = 256;
	bool g_has_texture_npot = false;
	int g_texture_filter = GL_LINEAR;

	void ProbeOpenGL()
	{
		// Called right after the context becomes current on every platform,
		// so this is where the batch can build its program.
		if (!g_batch.Initialize())
			LOG(Error, "OpenGL: vertex batch could not be initialized");

		GLint size;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
		g_max_texture_size = size;
		LOG(Info, "OpenGL: maximum texture size is " << size << "x" << size);

		std::string extensions = (const char*)glGetString(GL_EXTENSIONS);
		std::transform(extensions.begin(), extensions.end(), extensions.begin(), ::tolower);
		g_has_texture_npot = extensions.find("gl_arb_texture_non_power_of_two") != std::string::npos;
		LOG(Info, "OpenGL: GPU " << (g_has_texture_npot? "supports": "does not support") << " NPOTD textures");
	}
}

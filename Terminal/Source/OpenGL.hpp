/*
* BearLibTerminal
* Copyright (C) 2013-2016 Cfyz
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

#ifndef BEARLIBTERMINAL_OPENGL_HPP
#define BEARLIBTERMINAL_OPENGL_HPP

// Windows requires inclusion of windows.h since gl.h depends on it
// Also, gl.h in windows is a bit outdated
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#undef LoadBitmap // There is a function with same name in WinAPI
// OpenGL 1.2+
#define GL_BGRA 0x80E1
#elif defined(__ANDROID__)
// Android — не Linux в этом месте, хотя __linux там тоже определён: никакого
// GL/gl.h в NDK нет, есть GLES. Проверка на Android обязана стоять раньше.
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
// Четырёхугольников в GLES нет вовсе; значение нужно лишь как признак режима
// внутри VertexBatch, который всё равно рисует треугольниками.
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
#elif defined(__linux)
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#endif

/*
 * Entry points beyond OpenGL 1.1.
 *
 * Shaders are OpenGL 2.0, and on Windows opengl32.dll exports nothing past
 * 1.1 -- everything newer has to be asked for at runtime. Linux drivers do
 * export them, but going through the same loader on both keeps one code path
 * instead of two. On Android there is nothing to load: the GLES library has
 * these functions to begin with.
 *
 * Only what the terminal actually uses is declared. A full loader would be a
 * dependency to maintain for no gain.
 */
#if !defined(__ANDROID__)

#include <cstddef>

typedef char GLchar;
#if !defined(GL_VERSION_1_5)
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0

namespace BearLibTerminal
{
	typedef GLuint (*PFN_glCreateShader)(GLenum);
	typedef void (*PFN_glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
	typedef void (*PFN_glCompileShader)(GLuint);
	typedef void (*PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
	typedef void (*PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
	typedef void (*PFN_glDeleteShader)(GLuint);
	typedef GLuint (*PFN_glCreateProgram)();
	typedef void (*PFN_glAttachShader)(GLuint, GLuint);
	typedef void (*PFN_glLinkProgram)(GLuint);
	typedef void (*PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
	typedef void (*PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
	typedef void (*PFN_glUseProgram)(GLuint);
	typedef GLint (*PFN_glGetAttribLocation)(GLuint, const GLchar*);
	typedef GLint (*PFN_glGetUniformLocation)(GLuint, const GLchar*);
	typedef void (*PFN_glUniform1i)(GLint, GLint);
	typedef void (*PFN_glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
	typedef void (*PFN_glEnableVertexAttribArray)(GLuint);
	typedef void (*PFN_glDisableVertexAttribArray)(GLuint);
	typedef void (*PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
	typedef void (*PFN_glGenBuffers)(GLsizei, GLuint*);
	typedef void (*PFN_glBindBuffer)(GLenum, GLuint);
	typedef void (*PFN_glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
	typedef void (*PFN_glDeleteBuffers)(GLsizei, const GLuint*);

	extern PFN_glCreateShader bltCreateShader;
	extern PFN_glShaderSource bltShaderSource;
	extern PFN_glCompileShader bltCompileShader;
	extern PFN_glGetShaderiv bltGetShaderiv;
	extern PFN_glGetShaderInfoLog bltGetShaderInfoLog;
	extern PFN_glDeleteShader bltDeleteShader;
	extern PFN_glCreateProgram bltCreateProgram;
	extern PFN_glAttachShader bltAttachShader;
	extern PFN_glLinkProgram bltLinkProgram;
	extern PFN_glGetProgramiv bltGetProgramiv;
	extern PFN_glGetProgramInfoLog bltGetProgramInfoLog;
	extern PFN_glUseProgram bltUseProgram;
	extern PFN_glGetAttribLocation bltGetAttribLocation;
	extern PFN_glGetUniformLocation bltGetUniformLocation;
	extern PFN_glUniform1i bltUniform1i;
	extern PFN_glUniformMatrix4fv bltUniformMatrix4fv;
	extern PFN_glEnableVertexAttribArray bltEnableVertexAttribArray;
	extern PFN_glDisableVertexAttribArray bltDisableVertexAttribArray;
	extern PFN_glVertexAttribPointer bltVertexAttribPointer;
	extern PFN_glGenBuffers bltGenBuffers;
	extern PFN_glBindBuffer bltBindBuffer;
	extern PFN_glBufferData bltBufferData;
	extern PFN_glDeleteBuffers bltDeleteBuffers;

	// True when every entry point above was found.
	bool LoadOpenGLEntryPoints();
}

#else

#define bltCreateShader glCreateShader
#define bltShaderSource glShaderSource
#define bltCompileShader glCompileShader
#define bltGetShaderiv glGetShaderiv
#define bltGetShaderInfoLog glGetShaderInfoLog
#define bltDeleteShader glDeleteShader
#define bltCreateProgram glCreateProgram
#define bltAttachShader glAttachShader
#define bltLinkProgram glLinkProgram
#define bltGetProgramiv glGetProgramiv
#define bltGetProgramInfoLog glGetProgramInfoLog
#define bltUseProgram glUseProgram
#define bltGetAttribLocation glGetAttribLocation
#define bltGetUniformLocation glGetUniformLocation
#define bltUniform1i glUniform1i
#define bltUniformMatrix4fv glUniformMatrix4fv
#define bltEnableVertexAttribArray glEnableVertexAttribArray
#define bltDisableVertexAttribArray glDisableVertexAttribArray
#define bltVertexAttribPointer glVertexAttribPointer
#define bltGenBuffers glGenBuffers
#define bltBindBuffer glBindBuffer
#define bltBufferData glBufferData
#define bltDeleteBuffers glDeleteBuffers

namespace BearLibTerminal
{
	inline bool LoadOpenGLEntryPoints() { return true; }
}

#endif

namespace BearLibTerminal
{
	// OpenGL states/caps
	// This breaks strict opengl context ownership
	extern int g_max_texture_size;
	extern bool g_has_texture_npot;
	extern int g_texture_filter;

	// Порядок составляющих цвета, который принимает видеокарта.
	//
	// В памяти пиксели лежат BGRA — так объявлен Color. Настольный OpenGL
	// принимает такой порядок как есть, а в OpenGL ES формата GL_BGRA нет
	// вовсе: там либо расширение GL_EXT_texture_format_BGRA8888, либо
	// перестановка байтов перед загрузкой.
	extern bool g_has_bgra;

	void ProbeOpenGL();
}

#endif // BEARLIBTERMINAL_OPENGL_HPP

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

#include "VertexBatch.hpp"
#include "Log.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace BearLibTerminal
{
	VertexBatch g_batch;

	namespace
	{
		uint8_t ToByte(float value)
		{
			// Same normalization OpenGL applies to a float colour component:
			// round to nearest, clamp to the representable range. Truncation
			// instead of rounding would shift 0.5f alpha by one step and show
			// up as a different pixel.
			float scaled = value * 255.0f;
			if (scaled < 0.0f) scaled = 0.0f;
			if (scaled > 255.0f) scaled = 255.0f;
			return (uint8_t)std::lround(scaled);
		}
	}

	namespace
	{
		/* Shaders are written to the ES 1.00 profile: "attribute", "varying",
		 * gl_FragColor. That single source compiles on desktop GL 2.1 and up
		 * and on GLES 2 and 3 alike -- an ES3 context accepts ES2 shaders.
		 * Where ES3 features actually pay off (instancing a grid of glyphs)
		 * they can be added later; writing two dialects now would buy nothing
		 * and double what has to be kept in step. */
		const char* kVertexShader =
			"attribute vec2 a_position;\n"
			"attribute vec4 a_color;\n"
			"attribute vec2 a_texcoord;\n"
			"uniform mat4 u_projection;\n"
			"varying vec4 v_color;\n"
			"varying vec2 v_texcoord;\n"
			"void main()\n"
			"{\n"
			"    v_color = a_color;\n"
			"    v_texcoord = a_texcoord;\n"
			"    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);\n"
			"}\n";

		const char* kFragmentShader =
			"#ifdef GL_ES\n"
			"precision mediump float;\n"
			"#endif\n"
			"uniform sampler2D u_texture;\n"
			"uniform int u_textured;\n"
			"varying vec4 v_color;\n"
			"varying vec2 v_texcoord;\n"
			"void main()\n"
			"{\n"
			"    if (u_textured != 0)\n"
			"        gl_FragColor = texture2D(u_texture, v_texcoord) * v_color;\n"
			"    else\n"
			"        gl_FragColor = v_color;\n"
			"}\n";

		GLuint CompileShader(GLenum type, const char* source)
		{
			GLuint shader = bltCreateShader(type);
			bltShaderSource(shader, 1, &source, nullptr);
			bltCompileShader(shader);

			GLint status = 0;
			bltGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				GLint length = 0;
				bltGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
				std::vector<char> log(length > 1? length: 1, 0);
				bltGetShaderInfoLog(shader, (GLsizei)log.size(), nullptr, log.data());
				LOG(Error, "OpenGL: shader did not compile: " << log.data());
				bltDeleteShader(shader);
				return 0;
			}

			return shader;
		}
	}

	VertexBatch::VertexBatch():
		m_mode(GL_QUADS),
		m_started(false),
		m_quad_index(0),
		m_program(0),
		m_buffer(0),
		m_attrib_position(-1),
		m_attrib_color(-1),
		m_attrib_texcoord(-1),
		m_uniform_projection(-1),
		m_uniform_textured(-1),
		m_textured(false),
		m_ready(false)
	{
		for (int i = 0; i < 16; i++)
			m_projection[i] = (i % 5 == 0)? 1.0f: 0.0f;

		m_color[0] = m_color[1] = m_color[2] = m_color[3] = 255;
		m_texcoord[0] = m_texcoord[1] = 0.0f;
	}

	void VertexBatch::Begin(GLenum mode)
	{
		// A change of mode between Begin and Begin means the collected
		// geometry cannot be drawn in one call.
		if (m_started && mode != m_mode)
			Flush();

		m_mode = mode;
		m_started = true;
		m_quad_index = 0;
	}

	void VertexBatch::Color(int r, int g, int b, int a)
	{
		m_color[0] = (uint8_t)r;
		m_color[1] = (uint8_t)g;
		m_color[2] = (uint8_t)b;
		m_color[3] = (uint8_t)a;
	}

	void VertexBatch::Color(float r, float g, float b, float a)
	{
		Color(ToByte(r), ToByte(g), ToByte(b), ToByte(a));
	}

	void VertexBatch::TexCoord(float u, float v)
	{
		m_texcoord[0] = u;
		m_texcoord[1] = v;
	}

	void VertexBatch::Vertex(int x, int y)
	{
		if (m_mode == GL_LINES)
		{
			m_positions.push_back((float)x);
			m_positions.push_back((float)y);
			m_colors.insert(m_colors.end(), m_color, m_color + 4);
			m_texcoords.push_back(m_texcoord[0]);
			m_texcoords.push_back(m_texcoord[1]);
			return;
		}

		int i = m_quad_index;
		m_quad_positions[i*2 + 0] = (float)x;
		m_quad_positions[i*2 + 1] = (float)y;
		std::copy(m_color, m_color + 4, m_quad_colors + i*4);
		m_quad_texcoords[i*2 + 0] = m_texcoord[0];
		m_quad_texcoords[i*2 + 1] = m_texcoord[1];
		m_quad_index += 1;

		if (m_quad_index < 4)
			return;

		// Two triangles, wound the way GL splits a quad: 0-1-2 and 0-2-3.
		// Any other split changes how a four-colour gradient interpolates.
		static const int kOrder[6] = {0, 1, 2, 0, 2, 3};
		for (int k = 0; k < 6; k++)
		{
			int c = kOrder[k];
			m_positions.push_back(m_quad_positions[c*2 + 0]);
			m_positions.push_back(m_quad_positions[c*2 + 1]);
			m_colors.insert(m_colors.end(), m_quad_colors + c*4, m_quad_colors + c*4 + 4);
			m_texcoords.push_back(m_quad_texcoords[c*2 + 0]);
			m_texcoords.push_back(m_quad_texcoords[c*2 + 1]);
		}

		m_quad_index = 0;
	}

	bool VertexBatch::Initialize()
	{
		if (m_ready)
			return true;

		if (!LoadOpenGLEntryPoints())
		{
			LOG(Fatal, "OpenGL: shaders are not available, nothing can be drawn");
			return false;
		}

		if (!BuildProgram())
			return false;

		bltGenBuffers(1, &m_buffer);
		m_ready = true;
		return true;
	}

	bool VertexBatch::BuildProgram()
	{
		GLuint vertex = CompileShader(GL_VERTEX_SHADER, kVertexShader);
		if (!vertex)
			return false;

		GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader);
		if (!fragment)
		{
			bltDeleteShader(vertex);
			return false;
		}

		m_program = bltCreateProgram();
		bltAttachShader(m_program, vertex);
		bltAttachShader(m_program, fragment);
		bltLinkProgram(m_program);

		GLint status = 0;
		bltGetProgramiv(m_program, GL_LINK_STATUS, &status);
		if (!status)
		{
			GLint length = 0;
			bltGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> log(length > 1? length: 1, 0);
			bltGetProgramInfoLog(m_program, (GLsizei)log.size(), nullptr, log.data());
			LOG(Error, "OpenGL: program did not link: " << log.data());
			m_program = 0;
			return false;
		}

		// Shaders are kept by the program once linked.
		bltDeleteShader(vertex);
		bltDeleteShader(fragment);

		m_attrib_position = bltGetAttribLocation(m_program, "a_position");
		m_attrib_color = bltGetAttribLocation(m_program, "a_color");
		m_attrib_texcoord = bltGetAttribLocation(m_program, "a_texcoord");
		m_uniform_projection = bltGetUniformLocation(m_program, "u_projection");
		m_uniform_textured = bltGetUniformLocation(m_program, "u_textured");

		bltUseProgram(m_program);
		bltUniform1i(bltGetUniformLocation(m_program, "u_texture"), 0);

		return true;
	}

	void VertexBatch::SetProjection(float left, float right, float bottom, float top)
	{
		// The very matrix glOrtho used to build, with near/far at -1 and +1.
		float* m = m_projection;
		std::fill(m, m + 16, 0.0f);
		m[0]  = 2.0f / (right - left);
		m[5]  = 2.0f / (top - bottom);
		m[10] = -1.0f;
		m[12] = -(right + left) / (right - left);
		m[13] = -(top + bottom) / (top - bottom);
		m[15] = 1.0f;
	}

	void VertexBatch::SetTextured(bool textured)
	{
		// Geometry already collected was meant for the previous state.
		if (textured != m_textured)
			Flush();
		m_textured = textured;
	}

	void VertexBatch::End()
	{
		Flush();
		m_started = false;
	}

	void VertexBatch::Flush()
	{
		// An unfinished quad is dropped rather than guessed at: the old code
		// would have produced nothing for it either.
		m_quad_index = 0;

		if (m_positions.empty())
			return;

		if (!m_ready)
			return;

		GLsizei count = (GLsizei)(m_positions.size() / 2);

		// One buffer, refilled every flush. Interleaving the three attributes
		// would save two uploads, but it would also mix the assembly of the
		// geometry with its layout in memory, and that is the kind of change
		// worth making after the port works, not during it.
		size_t bytes_positions = m_positions.size() * sizeof(float);
		size_t bytes_colors = m_colors.size();
		size_t bytes_texcoords = m_texcoords.size() * sizeof(float);

		bltBindBuffer(GL_ARRAY_BUFFER, m_buffer);
		bltBufferData(GL_ARRAY_BUFFER,
			(GLsizeiptr)(bytes_positions + bytes_colors + bytes_texcoords),
			nullptr, GL_STREAM_DRAW);

		{
			// Sub-uploads would need glBufferSubData; filling a staging vector
			// keeps the entry point list shorter, and this runs once per batch,
			// not once per glyph.
			std::vector<uint8_t> staging;
			staging.reserve(bytes_positions + bytes_colors + bytes_texcoords);
			const uint8_t* p = (const uint8_t*)m_positions.data();
			staging.insert(staging.end(), p, p + bytes_positions);
			staging.insert(staging.end(), m_colors.begin(), m_colors.end());
			const uint8_t* t = (const uint8_t*)m_texcoords.data();
			staging.insert(staging.end(), t, t + bytes_texcoords);
			bltBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)staging.size(),
				staging.data(), GL_STREAM_DRAW);
		}

		bltUseProgram(m_program);
		bltUniformMatrix4fv(m_uniform_projection, 1, GL_FALSE, m_projection);
		bltUniform1i(m_uniform_textured, m_textured? 1: 0);

		bltEnableVertexAttribArray((GLuint)m_attrib_position);
		bltVertexAttribPointer((GLuint)m_attrib_position, 2, GL_FLOAT, GL_FALSE, 0,
			(const void*)0);
		bltEnableVertexAttribArray((GLuint)m_attrib_color);
		bltVertexAttribPointer((GLuint)m_attrib_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,
			(const void*)bytes_positions);
		bltEnableVertexAttribArray((GLuint)m_attrib_texcoord);
		bltVertexAttribPointer((GLuint)m_attrib_texcoord, 2, GL_FLOAT, GL_FALSE, 0,
			(const void*)(bytes_positions + bytes_colors));

		glDrawArrays(m_mode == GL_LINES? GL_LINES: GL_TRIANGLES, 0, count);

		bltDisableVertexAttribArray((GLuint)m_attrib_texcoord);
		bltDisableVertexAttribArray((GLuint)m_attrib_color);
		bltDisableVertexAttribArray((GLuint)m_attrib_position);
		bltBindBuffer(GL_ARRAY_BUFFER, 0);

		// Keep the capacity: the same amount of geometry arrives every frame,
		// and reallocating it sixty times a second would be pointless.
		m_positions.clear();
		m_colors.clear();
		m_texcoords.clear();
	}
}

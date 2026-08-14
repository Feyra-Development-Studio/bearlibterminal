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
#include <algorithm>
#include <cmath>

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

	VertexBatch::VertexBatch():
		m_mode(GL_QUADS),
		m_started(false),
		m_quad_index(0)
	{
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

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, m_positions.data());
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_colors.data());
		glTexCoordPointer(2, GL_FLOAT, 0, m_texcoords.data());

		glDrawArrays(m_mode == GL_LINES? GL_LINES: GL_TRIANGLES,
			0, (GLsizei)(m_positions.size() / 2));

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		// Keep the capacity: the same amount of geometry arrives every frame,
		// and reallocating it sixty times a second would be pointless.
		m_positions.clear();
		m_colors.clear();
		m_texcoords.clear();
	}
}

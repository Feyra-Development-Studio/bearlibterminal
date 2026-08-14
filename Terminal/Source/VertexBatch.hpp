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

#ifndef BEARLIBTERMINAL_VERTEXBATCH_HPP
#define BEARLIBTERMINAL_VERTEXBATCH_HPP

/*
 * Collecting geometry into arrays instead of feeding it vertex by vertex.
 *
 * The terminal used to draw through glBegin/glVertex/glEnd. That is OpenGL
 * 1.x fixed pipeline, and it does not exist in OpenGL ES at all -- neither in
 * ES2 nor in ES3 -- so an Android port is impossible while it stays.
 *
 * The call surface here deliberately mirrors the old one: Begin, Color,
 * TexCoord, Vertex, End. That keeps the drawing code untouched, which is the
 * whole point -- rendering is shared by every platform, and the surest way to
 * break the desktop picture is to rewrite the places that produce it while
 * also changing how they are drawn. One thing at a time.
 *
 * Quads are split into triangles the same way GL does it internally
 * (0,1,2 and 0,2,3), so a gradient across four corners keeps interpolating
 * exactly as before.
 */

#include "OpenGL.hpp"
#include <cstdint>
#include <vector>

namespace BearLibTerminal
{
	class VertexBatch
	{
	public:
		VertexBatch();

		// Mode is GL_QUADS or GL_LINES, matching what the call sites used.
		void Begin(GLenum mode);
		// Two overloads mirroring glColor4ub and glColor4f. Integer literals
		// would be ambiguous between them, so the byte form takes int: the
		// call sites pass plain ints and there is no reason to make them cast.
		void Color(int r, int g, int b, int a);
		void Color(float r, float g, float b, float a);
		void TexCoord(float u, float v);
		void Vertex(int x, int y);
		void End();

	private:
		void Flush();

		GLenum m_mode;
		bool m_started;

		// Current vertex attributes, applied to every vertex until changed --
		// exactly the semantics of glColor/glTexCoord.
		uint8_t m_color[4];
		float m_texcoord[2];

		std::vector<float> m_positions;
		std::vector<uint8_t> m_colors;
		std::vector<float> m_texcoords;

		// Corners of the quad being assembled.
		float m_quad_positions[8];
		uint8_t m_quad_colors[16];
		float m_quad_texcoords[8];
		int m_quad_index;
	};

	extern VertexBatch g_batch;
}

#endif // BEARLIBTERMINAL_VERTEXBATCH_HPP

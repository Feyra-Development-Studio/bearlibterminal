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

#include <stdexcept>
#include "Texture.hpp"
#include "VertexBatch.hpp"
#include "OpenGL.hpp"
#include "Log.hpp"
#include <vector>

namespace BearLibTerminal
{
	/* Порядок составляющих цвета при загрузке в текстуру.
	 *
	 * В памяти пиксель лежит как BGRA (см. объявление Color). Настольный
	 * OpenGL принимает такой порядок как есть. В OpenGL ES формата GL_BGRA
	 * нет: если устройство даёт расширение GL_EXT_texture_format_BGRA8888 —
	 * пользуемся им, иначе байты переставляются здесь, перед отправкой.
	 *
	 * Перестановка — обычный проход по растру. Делать её вычислениями на
	 * стороне JVM нельзя: библиотека написана на C++ и ни с какой виртуальной
	 * машиной не связана, а на настольных сборках её нет и в помине.
	 */
	static GLenum ColorFormat()
	{
		return g_has_bgra? GL_BGRA: GL_RGBA;
	}

	static GLint InternalFormat()
	{
		// В GLES внутренний формат для BGRA обязан совпадать с внешним,
		// в отличие от настольного GL, где хватает GL_RGBA8.
#if defined(__ANDROID__)
		return g_has_bgra? (GLint)GL_BGRA: (GLint)GL_RGBA;
#else
		return GL_RGBA8;
#endif
	}

	namespace
	{
		// Держится между вызовами, чтобы не выделять память заново на каждой
		// загрузке: текстуры грузятся редко, но растр бывает крупным.
		std::vector<uint8_t> g_swap_buffer;

		// Возвращает указатель на данные в том порядке, который примет
		// видеокарта. Если переставлять не нужно, отдаёт исходные данные.
		const uint8_t* ReorderIfNeeded(const uint8_t* data, size_t pixels)
		{
			if (g_has_bgra)
				return data;

			g_swap_buffer.resize(pixels * 4);
			for (size_t i = 0; i < pixels; i++)
			{
				g_swap_buffer[i*4 + 0] = data[i*4 + 2]; // r <- b
				g_swap_buffer[i*4 + 1] = data[i*4 + 1]; // g
				g_swap_buffer[i*4 + 2] = data[i*4 + 0]; // b <- r
				g_swap_buffer[i*4 + 3] = data[i*4 + 3]; // a
			}
			return g_swap_buffer.data();
		}
	}

	uint32_t Texture::m_currently_bound_handle{0};

	static bool IsPowerOfTwo(int value)
	{
		return (value != 0) && !(value & (value-1));
	}

	Texture::Texture():
		m_handle(0)
	{ }

	Texture::Texture(const Bitmap& bitmap):
		m_handle(0)
	{
		Update(bitmap);
	}

	Texture::Texture(Texture&& texture):
		m_handle(texture.m_handle),
		m_size(texture.m_size)
	{
		texture.m_size = Size();
		texture.m_handle = handle_t();
	}

	Texture::~Texture()
	{
		Dispose();
	}

	Texture& Texture::operator=(Texture&& texture)
	{
		Dispose();

		m_handle = texture.m_handle;
		m_size = texture.m_size;

		texture.m_size = Size();
		texture.m_handle = handle_t();

		return *this;
	}

	void Texture::Dispose()
	{
		if (m_handle > 0)
		{
			Unbind();
			glDeleteTextures(1, &m_handle);
			m_handle = 0;
		}
	}

	void Texture::Bind()
	{
		if (m_handle == 0)
		{
			LOG(Error, L"[Texture::Bind] texture is not allocated yet");
			throw std::runtime_error("invalid texture handle");
		}

		if (m_handle != m_currently_bound_handle)
		{
			glBindTexture(GL_TEXTURE_2D, m_handle);
			m_currently_bound_handle = m_handle;
		}
	}

	void Texture::Update(const Bitmap& bitmap)
	{
		// This class implements POTD texture only
		Size bitmap_size = bitmap.GetSize();
		if ((!IsPowerOfTwo(bitmap_size.width) || !IsPowerOfTwo(bitmap_size.height)) && !g_has_texture_npot)
		{
			LOG(Error, L"[Texture::Update] supplied bitmap is NPOTD");
			throw std::runtime_error("invalid bitmap");
		}

		if (m_handle == 0)
		{
			// Allocate a fresh texture object
			m_size = bitmap_size;
			glGenTextures(1, &m_handle);
			Bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_texture_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_texture_filter);
			glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat(), m_size.width, m_size.height, 0, ColorFormat(), GL_UNSIGNED_BYTE,
				ReorderIfNeeded((const uint8_t*)bitmap.GetData(), (size_t)m_size.Area()));
		}
		else
		{
			// Texture object already exists, update
			Bind();
			if (bitmap_size == m_size)
			{
				// Texture may be updated in-place
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size.width, m_size.height, ColorFormat(), GL_UNSIGNED_BYTE,
					ReorderIfNeeded((const uint8_t*)bitmap.GetData(), (size_t)m_size.Area()));
			}
			else
			{
				// Texture must be reallocated with new size (done by driver)
				m_size = bitmap_size;
				glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat(), m_size.width, m_size.height, 0, ColorFormat(), GL_UNSIGNED_BYTE,
				ReorderIfNeeded((const uint8_t*)bitmap.GetData(), (size_t)m_size.Area()));
			}
		}
	}

	Bitmap Texture::Download()
	{
		if (m_handle == 0)
		{
			LOG(Error, L"[Texture::Download] Texture is not yet created");
			throw std::runtime_error("invalid texture");
		}

		Bitmap result(m_size, Color());
		uint8_t* data = (uint8_t*)&result(0, 0);

		Bind();
#if defined(__ANDROID__)
		// glGetTexImage в OpenGL ES отсутствует вовсе. Обойти это можно
		// через кадровый буфер и glReadPixels, но заводить такое ради
		// вызова, которого в библиотеке нет ни одного, преждевременно.
		(void)data;
		LOG(Error, L"[Texture::Download] not available on this platform");
#else
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
#endif

		return result;
	}

	void Texture::Update(Rectangle area, const Bitmap& bitmap)
	{
		if (m_handle == 0)
		{
			throw std::runtime_error("Texture::Update(Rectangle, const Bitmap&): uninitialized texture");
		}

		if (area.Size() != bitmap.GetSize() || !Rectangle(m_size).Contains(area))
		{
			throw std::runtime_error("Texture::Update(Rectangle, const Bitmap&): invalid area");
		}

		Bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, area.left, area.top, area.width, area.height, ColorFormat(), GL_UNSIGNED_BYTE,
			ReorderIfNeeded((const uint8_t*)bitmap.GetData(), (size_t)(area.width * area.height)));
	}

	void Texture::ApplyTextureFilter()
	{
		if (m_handle != 0)
		{
			Bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_texture_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_texture_filter);
		}
	}

	Size Texture::GetSize() const
	{
		return m_size;
	}

	Texture::handle_t Texture::GetHandle() const
	{
		return m_handle;
	}

	void Texture::Enable()
	{
		// With shaders there is no GL_TEXTURE_2D state to switch: whether the
		// sampler is used is decided in the fragment shader.
		g_batch.SetTextured(true);
	}

	void Texture::Disable()
	{
		g_batch.SetTextured(false);
	}

	void Texture::Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		m_currently_bound_handle = 0;
	}

	Texture::handle_t Texture::BoundId()
	{
		return m_currently_bound_handle;
	}
}

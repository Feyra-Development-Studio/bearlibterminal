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

#ifndef BEARLIBTERMINAL_ANDROIDASSETS_HPP
#define BEARLIBTERMINAL_ANDROIDASSETS_HPP

#ifdef __ANDROID__

#include <cstdint>
#include <string>
#include <vector>

struct AAssetManager;

namespace BearLibTerminal
{
	/*
	 * Ресурсы приложения на Android лежат внутри APK, а не в файловой системе:
	 * привычного пути к ним не существует, и открыть их обычным потоком
	 * нельзя. Достать их можно только через AAssetManager, а он живёт в Java
	 * и передаётся приложением при запуске.
	 *
	 * Встроенных ресурсов библиотеки это не касается: шрифт по умолчанию и
	 * кодовые страницы зашиты прямо в код, поэтому терминал открывается и
	 * рисует даже когда никто ничего не передал.
	 */

	// Передаётся приложением, обычно один раз при запуске.
	void SetAssetManager(AAssetManager* manager);

	bool HasAssetManager();

	// Бросает std::runtime_error, если ресурса нет — как и чтение из файла,
	// чтобы вызывающему не приходилось различать источники.
	std::vector<uint8_t> ReadAsset(const std::wstring& name);
}

#endif // __ANDROID__

#endif // BEARLIBTERMINAL_ANDROIDASSETS_HPP

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

#include "AndroidAssets.hpp"
#include "Encoding.hpp"
#include "Log.hpp"
#include <android/asset_manager.h>
#include <stdexcept>

namespace BearLibTerminal
{
	namespace
	{
		AAssetManager* g_asset_manager = nullptr;
	}

	void SetAssetManager(AAssetManager* manager)
	{
		g_asset_manager = manager;
		LOG(Info, L"Android: asset manager has been " <<
			(manager? L"set": L"cleared"));
	}

	bool HasAssetManager()
	{
		return g_asset_manager != nullptr;
	}

	std::vector<uint8_t> ReadAsset(const std::wstring& name)
	{
		std::string path = UTF8Encoding().Convert(name);

		if (g_asset_manager == nullptr)
			throw std::runtime_error("asset \"" + path + "\" cannot be read: "
				"asset manager was not set by the application");

		AAsset* asset = AAssetManager_open(g_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
		if (asset == nullptr)
			throw std::runtime_error("asset \"" + path + "\" was not found");

		off_t size = AAsset_getLength(asset);
		std::vector<uint8_t> result((size_t)size);

		int read = AAsset_read(asset, result.data(), (size_t)size);
		AAsset_close(asset);

		if (read != (int)size)
			throw std::runtime_error("asset \"" + path + "\" was read partially");

		LOG(Debug, L"Loaded asset '" << name << L"' (" << (int)size << L" bytes)");
		return result;
	}
}

#endif // __ANDROID__

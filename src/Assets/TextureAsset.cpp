#include "Assets/TextureAsset.h"
#include "nlohmann/json.hpp"
#include "lz4.h"

using json = nlohmann::json;

namespace puffin::Assets
{
	////////////////////////////////
	// TextureAsset
	////////////////////////////////

	// Public

	bool TextureAsset::Save(TextureInfo& info, void* pixelData)
	{
		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = G_TEXTURE_TYPE;
		data.version = G_TEXTURE_VERSION;

		// Compress data into binary blob
		char* pixels = (char*)pixelData;

		int compressStaging = LZ4_compressBound(static_cast<int>(info.originalSize));

		data.binaryBlob.resize(compressStaging);

		int compressedSize = LZ4_compress_default(pixels, data.binaryBlob.data(), info.originalSize, compressStaging);

		float compressionRate = float(compressedSize) / float(info.originalSize);

		// If compression rate is more than 80% of original, it's not worth compressing the image
		if (compressionRate > 0.8)
		{
			compressedSize = info.originalSize;
			data.binaryBlob.resize(compressedSize);

			memcpy(data.binaryBlob.data(), pixels, compressedSize);
		}
		data.binaryBlob.resize(compressedSize);
		info.compressedSize = (uint32_t)compressedSize;

		// Fill Metadata from Info struct
		json metadata;
		metadata["textureFormat"] = "RGBA8";
		metadata["compression"] = "LZ4";
		metadata["original_file"] = info.originalFile;
		metadata["textureHeight"] = info.textureHeight;
		metadata["textureWidth"] = info.textureWidth;
		metadata["compressedSize"] = info.compressedSize;
		metadata["originalSize"] = info.originalSize;

		// Pass metadata to asset data struct
		data.json = metadata.dump();

		return SaveBinaryFile(fullPath, data);
	}

	bool TextureAsset::Load()
	{
		// Check if file is already loaded
		if (m_isLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();
		if (!fs::exists(fullPath))
			return false;
		
		// Load Binary/Metadata
		AssetData data;
		if (!LoadBinaryFile(fullPath, data))
		{
			return false;
		}

		// Parse Metadata from Json
		TextureInfo info = ParseTextureInfo(data);

		// Decompress Binary Data
		m_pixels.resize(info.originalSize);

		LZ4_decompress_safe(data.binaryBlob.data(), m_pixels.data(), 
		static_cast<int>(info.compressedSize), static_cast<int>(info.originalSize));

		m_texHeight = info.textureHeight;
		m_texWidth = info.textureWidth;

		m_texFormat = info.textureFormat;

		data.binaryBlob.clear();
		data.json.clear();

		m_isLoaded = true;
		return true;
	}

	void TextureAsset::Unload()
	{
		m_pixels.clear();
		m_pixels.shrink_to_fit();

		m_texWidth = 0;
		m_texHeight = 0;

		m_isLoaded = false;
	}

	// Private

	TextureInfo TextureAsset::ParseTextureInfo(const AssetData& data)
	{
		json metadata = json::parse(data.json);

		// Fill Texture Info struct with metadata
		TextureInfo info;

		std::string compressionMode = metadata["compression"];
		info.compressionMode = ParseCompressionMode(compressionMode.c_str());

		std::string textureFormat =  metadata["textureFormat"];
		info.textureFormat = ParseTextureFormat(textureFormat.c_str());

		info.originalFile = metadata["original_file"];
		info.textureHeight = metadata["textureHeight"];
		info.textureWidth = metadata["textureWidth"];
		info.compressedSize = metadata["compressedSize"];
		info.originalSize = metadata["originalSize"];

		return info;
	}
}
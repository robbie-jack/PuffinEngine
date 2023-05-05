#include "Assets/TextureAsset.h"
#include "nlohmann/json.hpp"
#include "lz4.h"

using json = nlohmann::json;

namespace puffin::assets
{
	////////////////////////////////
	// TextureAsset
	////////////////////////////////

	// Public

	bool TextureAsset::save(TextureInfo& info, void* pixelData)
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = gTextureType;
		data.version = gTextureVersion;

		// Compress data into binary blob
		const char* pixels = (char*)pixelData;

		const int compressStaging = LZ4_compressBound(static_cast<int>(info.originalSize));

		data.binaryBlob.resize(compressStaging);

		int compressedSize = LZ4_compress_default(pixels, data.binaryBlob.data(), info.originalSize, compressStaging);

		// If compression rate is more than 80% of original, it's not worth compressing the image
		if (const float compressionRate = static_cast<float>(compressedSize) / static_cast<float>(info.originalSize); compressionRate > 0.8)
		{
			compressedSize = info.originalSize;
			data.binaryBlob.resize(compressedSize);

			memcpy(data.binaryBlob.data(), pixels, compressedSize);
		}

		data.binaryBlob.resize(compressedSize);
		info.compressedSize = static_cast<uint32_t>(compressedSize);

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

		return saveBinaryFile(fullPath, data);
	}

	bool TextureAsset::load()
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();
		if (!fs::exists(fullPath))
			return false;
		
		// Load Binary/Metadata
		AssetData data;
		if (!loadBinaryFile(fullPath, data))
		{
			return false;
		}

		// Parse Metadata from Json
		const TextureInfo info = parseTextureInfo(data);

		// Decompress Binary Data
		mPixels.resize(info.originalSize);

		LZ4_decompress_safe(data.binaryBlob.data(), mPixels.data(), 
		static_cast<int>(info.compressedSize), static_cast<int>(info.originalSize));

		mTexHeight = info.textureHeight;
		mTexWidth = info.textureWidth;

		mTexFormat = info.textureFormat;

		data.binaryBlob.clear();
		data.json.clear();

		mIsLoaded = true;
		return true;
	}

	void TextureAsset::unload()
	{
		mPixels.clear();
		mPixels.shrink_to_fit();

		mTexWidth = 0;
		mTexHeight = 0;

		mIsLoaded = false;
	}

	// Private

	TextureInfo TextureAsset::parseTextureInfo(const AssetData& data) const
	{
		json metadata = json::parse(data.json);

		// Fill Texture Info struct with metadata
		TextureInfo info;

		const std::string compressionMode = metadata["compression"];
		info.compressionMode = parseCompressionMode(compressionMode.c_str());

		const std::string textureFormat =  metadata["textureFormat"];
		info.textureFormat = parseTextureFormat(textureFormat.c_str());

		info.originalFile = metadata["original_file"];
		info.textureHeight = metadata["textureHeight"];
		info.textureWidth = metadata["textureWidth"];
		info.compressedSize = metadata["compressedSize"];
		info.originalSize = metadata["originalSize"];

		return info;
	}
}
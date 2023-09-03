#include "Assets/TextureAsset.h"
#include "nlohmann/json.hpp"
#include "lz4.h"
#include "lz4hc.h"

using json = nlohmann::json;

namespace puffin::assets
{
	////////////////////////////////
	// TextureAsset
	////////////////////////////////

	// Public

	bool TextureAsset::save()
	{
		if (mIsLoaded)
		{
			TextureInfo info;
			info.originalFile = mOriginalFile;
			info.compressionMode = mCompressionMode;
			info.textureFormat = mTexFormat;
			info.textureWidth = mTexWidth;
			info.textureHeight = mTexHeight;
			info.textureChannels = mTexChannels;
			info.originalSize = info.textureHeight * info.textureWidth * info.textureChannels;

			return save(info, mPixels.data());
		}

		return false;
	}

	bool TextureAsset::save(TextureInfo& info, void* pixelData)
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::Texture;
		data.version = gTextureVersion;

		// Compress data into binary blob
		const char* pixels = static_cast<char*>(pixelData);

		// Covert pixel data to appropriate bc format


		const size_t compressStaging = LZ4_compressBound(static_cast<int>(info.originalSize));

		data.binaryBlob.resize(compressStaging);

		// Compress using default LZ4 mode
		//int compressedSize = LZ4_compress_default(pixels, data.binaryBlob.data(), info.originalSize, compressStaging);

		// Compress using HC LZ4 mode (higher compression ratio, takes longer to compress, doesn't effect decompression time)
		int compressedSize = LZ4_compress_HC(pixels, data.binaryBlob.data(), info.originalSize, compressStaging, LZ4HC_CLEVEL_DEFAULT);

		// If compression rate is more than 80% of original, it's not worth compressing the image
		if (const double compressionRate = static_cast<float>(compressedSize) / static_cast<float>(info.originalSize); compressionRate > 0.8 || compressedSize == 0)
		{
			data.binaryBlob.resize(info.originalSize);

			memcpy(data.binaryBlob.data(), pixels, info.originalSize);

			info.compressionMode = CompressionMode::None;
		}
		else
		{
			data.binaryBlob.resize(compressedSize);
		}

		// Fill Metadata from Info struct
		json metadata;
		metadata["compression"] = parseCompressionStringFromMode(info.compressionMode);
		metadata["textureFormat"] = parseTextureStringFromFormat(info.textureFormat);
		metadata["original_file"] = info.originalFile;
		metadata["textureHeight"] = info.textureHeight;
		metadata["textureWidth"] = info.textureWidth;
		metadata["textureChannels"] = info.textureChannels;
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

		if (info.compressionMode == CompressionMode::LZ4)
		{
			LZ4_decompress_safe(data.binaryBlob.data(), mPixels.data(),
				static_cast<int>(data.binaryBlob.size()), static_cast<int>(info.originalSize));
		}
		else
		{
			std::copy_n(data.binaryBlob.data(), static_cast<int>(info.originalSize), mPixels.data());
		}

		mOriginalFile = info.originalFile;
		mCompressionMode = info.compressionMode;
		mTexHeight = info.textureHeight;
		mTexWidth = info.textureWidth;
		mTexChannels = info.textureChannels;
		mTexFormat = info.textureFormat;
		mTexSize = info.originalSize;

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
		info.textureChannels = metadata["textureChannels"];
		info.originalSize = metadata["originalSize"];

		return info;
	}
}
#include "puffin/assets/textureasset.h"

#include "nlohmann/json.hpp"
#include "lz4.h"
#include "lz4hc.h"
#include "puffin/assets/assetregistry.h"

using json = nlohmann::json;

namespace puffin::assets
{
	////////////////////////////////
	// TextureAsset
	////////////////////////////////

	// Public

	TextureFormat ParseTextureFormat(const char* f)
	{
		if (strcmp(f, "RGBA8") == 0)
		{
			return TextureFormat::RGBA8;
		}

		if (strcmp(f, "R8") == 0)
		{
			return TextureFormat::R8;
		}

		if (strcmp(f, "RG8") == 0)
		{
			return TextureFormat::RG8;
		}

		if (strcmp(f, "RGB8") == 0)
		{
			return TextureFormat::RGB8;
		}

		if (strcmp(f, "BC4") == 0)
		{
			return TextureFormat::BC4;
		}

		if (strcmp(f, "BC5") == 0)
		{
			return TextureFormat::BC5;
		}

		if (strcmp(f, "BC6H") == 0)
		{
			return TextureFormat::BC6H;
		}

		if (strcmp(f, "BC7") == 0)
		{
			return TextureFormat::BC7;
		}

		return TextureFormat::Unknown;
	}

	const char* ParseTextureStringFromFormat(TextureFormat format)
	{
		return gTexFormatToString.at(format);
	}

	TextureAsset::TextureAsset(): Asset(fs::path())
	{
	}

	TextureAsset::TextureAsset(const fs::path& path): Asset(path)
	{
	}

	TextureAsset::TextureAsset(const UUID id, const fs::path& path): Asset(id, path)
	{
	}

	const std::string& TextureAsset::GetType() const
	{
		return gTextureType;
	}

	const uint32_t& TextureAsset::GetVersion() const
	{
		return gTextureVersion;
	}

	bool TextureAsset::Save()
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

			return Save(info, mPixels.data());
		}

		return false;
	}

	bool TextureAsset::Save(TextureInfo& info, void* pixelData)
	{
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.id = GetID();
		data.type = AssetType::Texture;
		data.version = gTextureVersion;

		// Compress data into binary blob
		const char* pixels = static_cast<char*>(pixelData);

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

			info.compressionMode = CompressionMode::Uncompressed;
		}
		else
		{
			data.binaryBlob.resize(compressedSize);
		}

		// Fill Metadata from Info struct
		data.jsonData["compression"] = info.compressionMode;
		data.jsonData["textureFormat"] = info.textureFormat;
		data.jsonData["original_file"] = info.originalFile;
		data.jsonData["textureHeight"] = info.textureHeight;
		data.jsonData["textureWidth"] = info.textureWidth;
		data.jsonData["textureChannels"] = info.textureChannels;
		data.jsonData["originalSize"] = info.originalSize;

		return SaveBinaryFile(fullPath, data);
	}

	bool TextureAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;
		
		// Load Binary/Metadata
		AssetData data;
		if (!LoadBinaryFile(fullPath, data, loadHeaderOnly))
		{
			return false;
		}

		// Parse Metadata from Json
		const TextureInfo info = ParseTextureInfo(data);

		if (loadHeaderOnly)
			return true;

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
		data.jsonData.clear();

		mIsLoaded = true;
		return true;
	}

	void TextureAsset::Unload()
	{
		mPixels.clear();
		mPixels.shrink_to_fit();

		mTexWidth = 0;
		mTexHeight = 0;

		mIsLoaded = false;
	}

	std::vector<char>& TextureAsset::Pixels()
	{
		return mPixels;
	}

	uint32_t TextureAsset::TextureWidth() const
	{
		return mTexWidth;
	}

	uint32_t TextureAsset::TextureHeight() const
	{
		return mTexHeight;
	}

	uint8_t TextureAsset::TextureChannels() const
	{
		return mTexChannels;
	}

	TextureFormat TextureAsset::TextureFormat() const
	{
		return mTexFormat;
	}

	uint64_t TextureAsset::TextureSize() const
	{
		return mTexSize;
	}

	// Private

	TextureInfo TextureAsset::ParseTextureInfo(const AssetData& data)
	{
		// Fill Texture Info struct with metadata
		TextureInfo info;

		info.compressionMode = data.jsonData["compression"];
		info.textureFormat = data.jsonData["textureFormat"];
		info.originalFile = data.jsonData["original_file"];
		info.textureHeight = data.jsonData["textureHeight"];
		info.textureWidth = data.jsonData["textureWidth"];
		info.textureChannels = data.jsonData["textureChannels"];
		info.originalSize = data.jsonData["originalSize"];

		return info;
	}
}

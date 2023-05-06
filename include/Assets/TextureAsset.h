#pragma once

#include "AssetRegistry.h"

namespace puffin::assets
{
	static const std::string gTextureType = "Texture";
	static constexpr uint32_t gTextureVersion = 1; // Latest version of Texture Asset Format

	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		RGBA8
	};

	// Map of texture format to number of bytes per pixel
	const static std::unordered_map<TextureFormat, uint32_t> gTexSizeMap =
	{
		{ TextureFormat::RGBA8, 4 }
	};

	static TextureFormat parseTextureFormat(const char* f)
	{
		if (strcmp(f, "RGBA8") == 0)
		{
			return TextureFormat::RGBA8;
		}
		else
		{
			return TextureFormat::Unknown;
		}
	}

	struct TextureInfo
	{
		CompressionMode compressionMode;
		std::string originalFile;
		TextureFormat textureFormat;
		uint32_t textureHeight;
		uint32_t textureWidth;
		uint32_t compressedSize;
		uint32_t originalSize;
	};

	class TextureAsset : public Asset
	{
	public:

		TextureAsset() : Asset(fs::path()) {}

		TextureAsset(const fs::path& path) : Asset(path) {}

		TextureAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

		~TextureAsset() override = default;

		[[nodiscard]] const std::string& type() const override
		{
			return gTextureType;
		}

		[[nodiscard]] const uint32_t& version() const override
		{
			return gTextureVersion;
		}

		bool save(TextureInfo& info, void* pixelData);

		bool load();

		void unload() override;

		const std::vector<char>& pixels() const
		{
			return mPixels;
		}

		const void* pixelData() const
		{
			return mPixels.data();
		}

		[[nodiscard]] uint32_t textureWidth() const
		{
			return mTexWidth;
		}

		[[nodiscard]] uint32_t textureHeight() const
		{
			return mTexHeight;
		}

		[[nodiscard]] uint32_t textureSizePerPixel() const
		{
			return gTexSizeMap.at(mTexFormat);
		}

		[[nodiscard]] TextureFormat textureFormat() const
		{
			return mTexFormat;
		}

		[[nodiscard]] uint32_t textureSize() const
		{
			return mTexWidth * mTexHeight * gTexSizeMap.at(mTexFormat);
		}

	private:

		std::vector<char> mPixels;
		uint32_t mTexWidth;
		uint32_t mTexHeight;
		TextureFormat mTexFormat;

		TextureInfo parseTextureInfo(const AssetData& data) const;
	};
}
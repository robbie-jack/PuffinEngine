#pragma once

#include "AssetRegistry.h"

namespace puffin::assets
{
	static const std::string gTextureType = "Texture";
	static constexpr uint32_t gTextureVersion = 1; // Latest version of Texture Asset Format

	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		R8 = 1,
		RG8 = 2,
		RGB8 = 3,
		RGBA8 = 4
	};

	// Map of texture format to number of channels per pixel
	const static std::unordered_map<TextureFormat, uint8_t> gTexFormatToChannels =
	{
		{ TextureFormat::R8, 1 },
		{ TextureFormat::RG8, 2 },
		{ TextureFormat::RGB8, 3 },
		{ TextureFormat::RGBA8, 4 }
	};

	const static std::unordered_map<TextureFormat, const char*> gTexFormatToString =
	{
		{ TextureFormat::R8, "R8" },
		{ TextureFormat::RG8, "RG8" },
		{ TextureFormat::RGB8, "RGB8" },
		{ TextureFormat::RGBA8, "RGBA8" }
	};

	const static std::unordered_map<uint8_t, TextureFormat> gTexChannelsToFormat =
	{
		{ 1, TextureFormat::R8 },
		{ 2, TextureFormat::RG8 },
		{ 3, TextureFormat::RGB8 },
		{ 4, TextureFormat::RGBA8 }
	};

	static TextureFormat parseTextureFormat(const char* f)
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

		return TextureFormat::Unknown;
	}

	static const char* parseTextureStringFromFormat(TextureFormat format)
	{
		return gTexFormatToString.at(format);
	}

	struct TextureInfo
	{
		CompressionMode compressionMode;
		std::string originalFile;
		TextureFormat textureFormat;
		uint32_t textureHeight;
		uint32_t textureWidth;
		uint8_t textureChannels;
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

		bool save() override;

		bool save(TextureInfo& info, void* pixelData);

		bool load() override;

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

		[[nodiscard]] uint8_t textureChannels() const
		{
			return mTexChannels;
		}

		[[nodiscard]] uint32_t textureSizePerPixel() const
		{
			return gTexFormatToChannels.at(mTexFormat);
		}

		[[nodiscard]] TextureFormat textureFormat() const
		{
			return mTexFormat;
		}

		[[nodiscard]] uint32_t textureSize() const
		{
			return mTexWidth * mTexHeight * gTexFormatToChannels.at(mTexFormat);
		}

	private:

		std::vector<char> mPixels;
		uint32_t mTexWidth = 0;
		uint32_t mTexHeight = 0;
		uint8_t mTexChannels = 0;
		TextureFormat mTexFormat;

		std::string mOriginalFile;

		TextureInfo parseTextureInfo(const AssetData& data) const;
	};
}
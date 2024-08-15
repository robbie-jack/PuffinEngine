#pragma once

#include <string>
#include <unordered_map>

#include "puffin/assets/asset.h"

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
		RGBA8 = 4,
		BC4 = 5,
		BC5 = 6,
		BC6H = 7,
		BC7 = 8
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TextureFormat,
	{
		{ TextureFormat::Unknown, "Unknown"},
		{ TextureFormat::R8, "R8" },
		{ TextureFormat::RG8, "RG8" },
		{ TextureFormat::RGB8, "RGB8" },
		{ TextureFormat::RGBA8, "RGBA8" },
		{ TextureFormat::BC4, "BC4" },
		{ TextureFormat::BC5, "BC5" },
		{ TextureFormat::BC6H, "BC6H" },
		{ TextureFormat::BC7, "BC7" }
	});

	// Map of texture format to number of channels per pixel
	const static std::unordered_map<TextureFormat, const char*> gTexFormatToString =
	{
		{ TextureFormat::R8, "R8" },
		{ TextureFormat::RG8, "RG8" },
		{ TextureFormat::RGB8, "RGB8" },
		{ TextureFormat::RGBA8, "RGBA8" },
		{ TextureFormat::BC4, "BC4" },
		{ TextureFormat::BC5, "BC5" },
		{ TextureFormat::BC6H, "BC6H" },
		{ TextureFormat::BC7, "BC7" }
	};

	const static std::unordered_map<uint8_t, TextureFormat> gTexChannelsToRGBAFormat =
	{
		{ 1, TextureFormat::R8 },
		{ 2, TextureFormat::RG8 },
		{ 3, TextureFormat::RGB8 },
		{ 4, TextureFormat::RGBA8 }
	};

	const static std::unordered_map<uint8_t, TextureFormat> gTexChannelsToBCFormat =
	{
		{ 1, TextureFormat::BC4 },
		{ 2, TextureFormat::BC5 },
		{ 3, TextureFormat::BC7 },
		{ 4, TextureFormat::BC7 }
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

	static const char* parseTextureStringFromFormat(TextureFormat format)
	{
		return gTexFormatToString.at(format);
	}

	struct TextureInfo : AssetInfo
	{
		TextureFormat textureFormat;
		uint32_t textureHeight;
		uint32_t textureWidth;
		uint8_t textureChannels;
		uint32_t originalSize;
	};

	class TextureAsset : public Asset
	{
	public:

		TextureAsset() : Asset(fs::path()) {}

		TextureAsset(const fs::path& path) : Asset(path) {}

		TextureAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

		~TextureAsset() override = default;

		[[nodiscard]] const std::string& GetType() const override
		{
			return gTextureType;
		}

		[[nodiscard]] const uint32_t& GetVersion() const override
		{
			return gTextureVersion;
		}

		bool Save() override;

		bool save(TextureInfo& info, void* pixelData);

		bool Load(bool loadHeaderOnly = false) override;

		void Unload() override;

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

		[[nodiscard]] TextureFormat textureFormat() const
		{
			return mTexFormat;
		}

		[[nodiscard]] uint64_t textureSize() const
		{
			return mTexSize;
		}

	private:

		std::vector<char> mPixels;
		uint32_t mTexWidth = 0;
		uint32_t mTexHeight = 0;
		uint8_t mTexChannels = 0;
		uint64_t mTexSize = 0;
		TextureFormat mTexFormat;

		std::string mOriginalFile;
		CompressionMode mCompressionMode;

		static TextureInfo parseTextureInfo(const AssetData& data);
	};
}
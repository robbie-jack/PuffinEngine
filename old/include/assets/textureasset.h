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

	TextureFormat ParseTextureFormat(const char* f);

	static const char* ParseTextureStringFromFormat(TextureFormat format);

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

		TextureAsset();
		explicit TextureAsset(const fs::path& path);
		TextureAsset(const UUID id, const fs::path& path);

		~TextureAsset() override = default;

		[[nodiscard]] const std::string& GetType() const override;
		[[nodiscard]] const uint32_t& GetVersion() const override;

		bool Save() override;
		bool Save(TextureInfo& info, void* pixelData);

		bool Load(bool loadHeaderOnly = false) override;

		void Unload() override;

		[[nodiscard]] std::vector<char>& Pixels();

		[[nodiscard]] uint32_t TextureWidth() const;
		[[nodiscard]] uint32_t TextureHeight() const;
		[[nodiscard]] uint8_t TextureChannels() const;
		[[nodiscard]] TextureFormat TextureFormat() const;
		[[nodiscard]] uint64_t TextureSize() const;

	private:

		std::vector<char> mPixels;	
		uint32_t mTexWidth = 0;
		uint32_t mTexHeight = 0;
		uint8_t mTexChannels = 0;
		uint64_t mTexSize = 0;
		enum TextureFormat mTexFormat;

		std::string mOriginalFile;
		CompressionMode mCompressionMode;

		static TextureInfo ParseTextureInfo(const AssetData& data);
	};
}
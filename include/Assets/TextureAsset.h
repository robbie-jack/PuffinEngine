#pragma once

#include "AssetRegistry.h"

namespace Puffin::Assets
{
	static const std::string G_TEXTURE_TYPE = "Texture";
	static constexpr uint32_t G_TEXTURE_VERSION = 1; // Latest version of Texture Asset Format

	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		RGBA8
	};

	// Map of texture format to number of bytes per pixel
	const static std::unordered_map<TextureFormat, uint32_t> g_texSizeMap =
	{
		{ TextureFormat::RGBA8, 4 }
	};

	static TextureFormat ParseTextureFormat(const char* f)
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

		TextureAsset(const UUID id, const fs::path& path) : Asset(id, path) {}

		~TextureAsset() override = default;

		const std::string& Type() const
		{
			return G_TEXTURE_TYPE;
		}

		const uint32_t& Version() const
		{
			return G_TEXTURE_VERSION;
		}

		bool Save(TextureInfo& info, void* pixelData);

		bool Load();

		void Unload() override;

		const std::vector<char>& GetPixels() const
		{
			return m_pixels;
		}

		const void* GetPixelData() const
		{
			return m_pixels.data();
		}

		const uint32_t GetTextureWidth() const
		{
			return m_texWidth;
		}

		const uint32_t GetTextureHeight() const
		{
			return m_texHeight;
		}

		const uint32_t GetTexturePixelSize() const
		{
			return g_texSizeMap.at(m_texFormat);
		}

		const TextureFormat GetTextureFormat() const
		{
			return m_texFormat;
		}

		const uint32_t GetTextureSize() const
		{
			return m_texWidth * m_texHeight * g_texSizeMap.at(m_texFormat);
		}

	private:

		std::vector<char> m_pixels;
		uint32_t m_texWidth;
		uint32_t m_texHeight;
		TextureFormat m_texFormat;

		TextureInfo ParseTextureInfo(const AssetData& data);
	};
}
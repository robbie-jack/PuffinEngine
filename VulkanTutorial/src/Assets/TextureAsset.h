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

	static TextureFormat ParseTextureFormat(const char* f)
	{
		if (strcmp(f, "RGBA8"))
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

		void Unload();

		const std::vector<char>& GetPixels() const
		{
			return m_pixels;
		}

		const uint32_t GetTextureWidth() const
		{
			return m_textureWidth;
		}

		const uint32_t GetTextureHeight() const
		{
			return m_textureHeight;
		}

	private:

		std::vector<char> m_pixels;
		uint32_t m_textureWidth;
		uint32_t m_textureHeight;

		TextureInfo ParseTextureInfo(const AssetData& data);
	};
}
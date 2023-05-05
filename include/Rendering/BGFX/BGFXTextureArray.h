#pragma once

#include "bgfx/bgfx.h"
#include "bx/bx.h"
#include "Types/UUID.h"

#include "Assets/TextureAsset.h"

#include <unordered_map>


namespace puffin::rendering
{
	class TextureArray
	{
	public:

		TextureArray() = default;

		~TextureArray()
		{
			bgfx::destroy(mHandle);
			mNumTextures = 0;
		}

		void init(uint16_t texWidth = 2048, uint16_t texHeight = 2048, uint16_t numTextures = 16, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8)
		{
			mNumTextures = numTextures;
			mFormat = format;

			mTexWidth = texWidth;
			mTexHeight = texHeight;

			mHandle = bgfx::createTexture2D(mTexWidth, mTexHeight,
				false, mNumTextures, mFormat, 0, nullptr);
		}

		bool addTexture(const UUID uuid)
		{
			if (mIdToLayers.count(uuid) == 1)
			{
				return true;
			}

			if (const auto texAsset = std::static_pointer_cast<assets::TextureAsset>(assets::AssetRegistry::get()->getAsset(uuid)); texAsset && texAsset->load())
			{
				// Copy texture data into memory
				const uint32_t texSize = texAsset->textureSize();

				const bgfx::Memory* mem = bgfx::alloc(texSize);

				bx::memCopy(mem->data, texAsset->pixelData(), mem->size);

				// Add texture to array
				bgfx::updateTexture2D(mHandle, 0, mNumActiveTextures, 0, 0,
										texAsset->textureWidth(), texAsset->textureHeight(), mem);

				mIdToLayers[uuid] = mNumActiveTextures;
				mNumActiveTextures++;

				texAsset->unload();

				return true;
			}

			return false;
		}

		[[nodiscard]] uint16_t getTextureIndex(const UUID uuid) const { return mIdToLayers.at(uuid); }

		[[nodiscard]] bgfx::TextureHandle handle() const { return mHandle; }

	private:

		bgfx::TextureHandle mHandle = {};
		bgfx::TextureFormat::Enum mFormat;
		uint16_t mTexWidth = 0, mTexHeight = 0;

		uint16_t mNumTextures = 0; // Number of textures in array
		uint16_t mNumActiveTextures = 0; // Number of textures in array currently in use

		std::unordered_map<UUID, uint16_t> mIdToLayers;



	};

}

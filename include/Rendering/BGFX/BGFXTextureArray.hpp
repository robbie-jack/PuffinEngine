#pragma once

#include "bgfx/bgfx.h"
#include "bx/bx.h"
#include "Types/UUID.h"

#include "Assets/TextureAsset.h"

#include <unordered_map>


namespace Puffin::Rendering::BGFX
{
	class TextureArray
	{
	public:

		TextureArray() = default;

		~TextureArray()
		{
			bgfx::destroy(m_handle);
			m_numTextures = 0;
		}

		void Init(uint16_t texWidth = 2048, uint16_t texHeight = 2048, uint16_t numTextures = 16, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8)
		{
			m_numTextures = numTextures;
			m_format = format;

			m_texWidth = texWidth;
			m_texHeight = texHeight;

			m_handle = bgfx::createTexture2D(m_texWidth, m_texHeight,
				false, m_numTextures, m_format, 0, 0);
		}

		bool AddTexture(UUID uuid)
		{
			if (m_idtoLayersMap.count(uuid) == 1)
			{
				return true;
			}

			const auto texAsset = std::static_pointer_cast<Assets::TextureAsset>(Assets::AssetRegistry::Get()->GetAsset(uuid));

			if (texAsset && texAsset->Load())
			{
				// Copy texture data into memory
				uint32_t texSize = texAsset->GetTextureSize();

				const bgfx::Memory* mem = bgfx::alloc(texSize);

				bx::memCopy(mem->data, texAsset->GetPixelData(), mem->size);

				// Add texture to array
				bgfx::updateTexture2D(m_handle, m_numActiveTextures, 0, 0, 0, 
										texAsset->GetTextureWidth(), texAsset->GetTextureHeight(), mem);

				m_idtoLayersMap[uuid] = m_numActiveTextures;
				m_numActiveTextures++;

				texAsset->Unload();
			}

			return false;
		}

		[[nodiscard]] uint16_t GetTextureIndex(UUID uuid) const { return m_idtoLayersMap.at(uuid); }

		[[nodiscard]] bgfx::TextureHandle Handle() const { return m_handle; }

	private:

		bgfx::TextureHandle m_handle = {};
		bgfx::TextureFormat::Enum m_format;
		uint16_t m_texWidth = 0, m_texHeight = 0;

		uint16_t m_numTextures = 0; // Number of textures in array
		uint16_t m_numActiveTextures = 0; // Number of textures in array currently in use

		std::unordered_map<UUID, uint16_t> m_idtoLayersMap;



	};

}

#pragma once

#include <unordered_set>

#include "puffin/assets/textureasset.h"
#include "puffin/types/uuid.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/rendering/vulkan/typesvk.h"

namespace puffin::rendering
{
	class RenderSubsystemVK;

	const static std::unordered_map<assets::TextureFormat, vk::Format> gTexFormatVK =
	{
		{ assets::TextureFormat::R8, vk::Format::eR8Unorm },
		{ assets::TextureFormat::RG8, vk::Format::eR8G8Unorm },
		{ assets::TextureFormat::RGB8, vk::Format::eR8G8B8Unorm },
		{ assets::TextureFormat::RGBA8, vk::Format::eR8G8B8A8Unorm },
		{ assets::TextureFormat::BC4, vk::Format::eBc4UnormBlock },
		{ assets::TextureFormat::BC5, vk::Format::eBc5UnormBlock },
		{ assets::TextureFormat::BC7, vk::Format::eBc7UnormBlock }
	};

	class TextureManagerVK
	{
	public:

		explicit TextureManagerVK(puffin::rendering::RenderSubsystemVK* renderSubystem);
		~TextureManagerVK();

		void AddTextureToLoad(UUID assetID, bool forceReload = false);
		void AddTexturesToLoad(const std::vector<UUID>& assetIDs, bool forceReload = false);

		void AddTextureToUnload(UUID assetID);

		void LoadAndUnloadTextures();

		[[nodiscard]] bool IsLoaded(UUID assetID) const;
		[[nodiscard]] const Texture& GetTexture(UUID assetID) const;

		[[nodiscard]] bool TextureDescriptorNeedsUpdated() const;

		const std::unordered_map<UUID, Texture>& GetLoadedTextures() const;

	private:
		bool LoadTextureInternal(UUID assetID);

		void UnloadTextureInternal(UUID assetID) const;

		RenderSubsystemVK* mRenderSubsystem = nullptr;

		std::unordered_map<UUID, Texture> mTextures;
		std::unordered_set<UUID> mTexturesToLoad;
		std::unordered_set<UUID> mTexturesToUnload;
		bool mTextureDescriptorNeedsUpdated = false;
		
	};
}

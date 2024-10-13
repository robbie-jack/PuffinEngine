#pragma once

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

		bool LoadTexture(UUID assetID, bool forceReload = false);
		bool LoadTextures(const std::vector<UUID>& assetIDs, bool forceReload = false);

		bool UnloadTexture(UUID assetID);

	private:

		void DestroyTexture(const Texture& texture);

		RenderSubsystemVK* mRenderSubsystem = nullptr;

		std::unordered_map<UUID, Texture> mTextures;

	};
}

#include "puffin/rendering/vulkan/texturemanagervk.h"

#include "puffin/assets/assetregistry.h"
#include "puffin/assets/textureasset.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	TextureManagerVK::TextureManagerVK(puffin::rendering::RenderSubsystemVK* renderSubystem)
		: mRenderSubsystem(renderSubystem)
	{

	}

	TextureManagerVK::~TextureManagerVK()
	{
		for (const auto& [assetID, texture] : mTextures)
		{
			DestroyTexture(texture);
		}

		mTextures.clear();
	}

	bool TextureManagerVK::LoadTexture(UUID assetID, bool forceReload)
	{
		// Check if texture is already loaded
		if (mTextures.find(assetID) != mTextures.end() && !forceReload)
			return true;

		// Load texture
		const auto texAsset = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(assetID);
		if (!texAsset || !texAsset->Load())
			return false;

		if (forceReload)
		{
			UnloadTexture(assetID);
		}

		// Init new texture
		util::InitTextureParams params = {};
		params.pixelData = texAsset->Pixels().data();
		params.width = texAsset->TextureWidth();
		params.height = texAsset->TextureHeight();
		params.dataSize = texAsset->TextureSize();
		params.format = gTexFormatVK.at(texAsset->TextureFormat());
		mTextures.emplace(assetID, util::InitTexture(mRenderSubsystem, params));

		// Unload asset once finished
		texAsset->Unload();

		return true;
	}

	bool TextureManagerVK::LoadTextures(const std::vector<UUID>& assetIDs, bool forceReload)
	{
		bool allTexturesLoaded = true;
		for (const auto& assetID : assetIDs)
		{
			if (!LoadTexture(assetID))
				allTexturesLoaded = false;
		}

		return allTexturesLoaded;
	}

	bool TextureManagerVK::UnloadTexture(UUID assetID)
	{
		if (mTextures.find(assetID) == mTextures.end())
			return true;

		DestroyTexture(mTextures.at(assetID));

		mTextures.erase(assetID);

		return true;
	}

	void TextureManagerVK::DestroyTexture(const Texture& texture)
	{
		mRenderSubsystem->GetDevice().destroyImageView(texture.imageView);
		mRenderSubsystem->GetAllocator().destroyImage(texture.image, texture.allocation);
	}
}

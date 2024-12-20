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
			UnloadTextureInternal(assetID);
		}

		mTextures.clear();
	}

	void TextureManagerVK::AddTextureToLoad(UUID assetID, bool forceReload)
	{
		// Check if texture is already loaded
		if (mTextures.find(assetID) != mTextures.end() && !forceReload)
			return;

		if (forceReload)
			mTexturesToUnload.emplace(assetID);
		
		mTexturesToLoad.emplace(assetID);
	}

	void TextureManagerVK::AddTexturesToLoad(const std::vector<UUID>& assetIDs, bool forceReload)
	{
		for (const auto& assetID : assetIDs)
		{
			AddTextureToLoad(assetID, forceReload);
		}
	}

	void TextureManagerVK::AddTextureToUnload(UUID assetID)
	{
		if (mTextures.find(assetID) == mTextures.end())
			return;

		mTexturesToUnload.emplace(assetID);
	}

	void TextureManagerVK::LoadAndUnloadTextures()
	{
		mTextureDescriptorNeedsUpdated = false;
		
		for (const UUID& assetID : mTexturesToUnload)
		{
			if (IsLoaded(assetID))
			{
				UnloadTextureInternal(assetID);
				mTextures.erase(assetID);
			}
		}
		
		for (const UUID& assetID : mTexturesToLoad)
		{
			if (LoadTextureInternal(assetID) && !mTextureDescriptorNeedsUpdated)
				mTextureDescriptorNeedsUpdated = true;
		}
	}

	bool TextureManagerVK::IsLoaded(UUID assetID) const
	{
		return mTextures.find(assetID) != mTextures.end();
	}

	const Texture& TextureManagerVK::GetTexture(UUID assetID) const
	{
		return mTextures.at(assetID);
	}

	bool TextureManagerVK::TextureDescriptorNeedsUpdated() const
	{
		return mTextureDescriptorNeedsUpdated;
	}

	const std::unordered_map<UUID, Texture>& TextureManagerVK::GetLoadedTextures() const
	{
		return mTextures;
	}

	bool TextureManagerVK::LoadTextureInternal(UUID assetID)
	{
		// Load texture
		const auto texAsset = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(assetID);
		if (!texAsset || !texAsset->Load())
			return false;

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

	void TextureManagerVK::UnloadTextureInternal(UUID assetID) const
	{
		const auto& texture = mTextures.at(assetID);
		
		mRenderSubsystem->GetDevice().destroyImageView(texture.imageView);
		mRenderSubsystem->GetAllocator().destroyImage(texture.image, texture.allocation);
	}
}

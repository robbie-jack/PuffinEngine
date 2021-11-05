#include <AssetRegistry.h>

namespace Puffin
{
	AssetRegistry* AssetRegistry::instance = 0;

	void AssetRegistry::ProjectRoot(fs::path projectRootPath)
	{
		projectRootPath_ = projectRootPath;
	}

	fs::path AssetRegistry::ProjectRoot()
	{
		return projectRootPath_;
	}

	fs::path AssetRegistry::ContentRoot()
	{
		return projectRootPath_ / "content";
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::GetAsset(const UUID& uuid)
	{
		return idToAssetMap_[uuid];
	}
	
	// Get Typed Asset from Registry
	template<typename AssetType>
	std::shared_ptr<AssetType> AssetRegistry::GetAsset(const UUID& uuid)
	{
		return std::static_pointer_cast<AssetType>(idToAssetMap_[uuid]);
	}

	// Register new Asset to Registry
	UUID AssetRegistry::RegisterAsset(std::shared_ptr<Asset> asset)
	{
		idToAssetMap_.insert({ asset->ID(), asset });
		return asset->ID();
	}
}
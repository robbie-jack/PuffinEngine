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
		// Return asset if it has been registered
		if (idToAssetMap_.count(uuid))
		{
			return idToAssetMap_[uuid];
		}

		// No asset with that ID has been registered, return nullptr
		return nullptr;
	}

	std::shared_ptr<Asset> AssetRegistry::GetAsset(const fs::path& path)
	{
		// Return asset if it has been registered
		if (pathToIDMap_.count(path.string()))
		{
			return GetAsset(pathToIDMap_[path.string()]);
		}
		
		// No asset with that path has been registered, return nullptr
		return nullptr;
	}
}
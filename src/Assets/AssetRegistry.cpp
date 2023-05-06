#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"

#include <fstream>

namespace puffin::assets
{
	AssetRegistry* AssetRegistry::sInstance = nullptr;

	void AssetRegistry::setProjectName(const std::string& projectName)
	{
		mProjectName = projectName;
	}

	std::string AssetRegistry::projectName()
	{
		return mProjectName;
	}

	void AssetRegistry::setProjectRoot(fs::path projectRootPath)
	{
		mProjectRootPath = projectRootPath;
	}

	fs::path AssetRegistry::setProjectRoot()
	{
		return mProjectRootPath;
	}

	fs::path AssetRegistry::contentRoot() const
	{
		return mProjectRootPath / "content";
	}

	void AssetRegistry::saveAssetCache() const
	{
		// Fill AssetCache Struct
		AssetCache assetCache;

		for (const auto& [fst, snd] : mIdToAssetMap)
		{
			assetCache.data[fst].path = snd->relativePath().string();
			assetCache.data[fst].type = snd->type();
		}

		json data = assetCache;

		// Initialize Output File Stream and Cereal Binary Archive
		const fs::path assetCachePath = mProjectRootPath / (mProjectName + ".passetcache");
		std::ofstream os(assetCachePath, std::ios::out);

		os << std::setw(4) << data << std::endl;

		os.close();
	}

	void AssetRegistry::loadAssetCache()
	{
		// Initialize Input File Stream and Cereal Binary Archive
		const fs::path assetCachePath = mProjectRootPath / (mProjectName + ".passetcache");
		std::ifstream is(assetCachePath);

		json data;
		is >> data;

		// Load AssetCache from file
		AssetCache assetCache = data;

		// Create Assets from factories based on their stored type
		for (const auto& [fst, snd] : assetCache.data)
		{
			std::string path = snd.path;
			std::string type = snd.type;
			std::shared_ptr<Asset> asset;

			for (const auto& factory : mAssetFactories)
			{
				if (factory->type() == type)
				{
					asset = factory->addAsset(fst, path);
					break;
				}
			}

			mIdToAssetMap[asset->id()] = asset;
			mPathToIdMap[asset->relativePath().string()] = asset->id();
		}
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::getAsset(const PuffinId& uuid)
	{
		// Return asset if it has been registered
		if (mIdToAssetMap.count(uuid))
		{
			return mIdToAssetMap[uuid];
		}

		// No asset with that ID has been registered, return nullptr
		return nullptr;
	}

	std::shared_ptr<Asset> AssetRegistry::getAsset(const fs::path& path)
	{
		// Return asset if it has been registered
		if (mPathToIdMap.count(path.string()))
		{
			return getAsset(mPathToIdMap[path.string()]);
		}
		
		// No asset with that path has been registered, return nullptr
		return nullptr;
	}
}
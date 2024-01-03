#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"

#include <fstream>

namespace puffin::assets
{
	AssetRegistry* AssetRegistry::sInstance = nullptr;

	void AssetRegistry::init(const io::ProjectFile& projectFile, const fs::path& projectPath)
	{
		mProjectName = projectFile.name;
		mProjectRootPath = projectPath;
		mProjectRootPath.remove_filename();

		mEngineRootPath = findEngineRoot(fs::current_path());

		mContentDirectories.push_back(mProjectRootPath / "content");

		for (auto& dir : projectFile.additionalContentDirectories)
		{
			mContentDirectories.emplace_back(dir);
		}
	}

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

	fs::path AssetRegistry::projectRoot()
	{
		return mProjectRootPath;
	}

	fs::path AssetRegistry::contentRoot() const
	{
		return mContentDirectories[0];
	}

	fs::path AssetRegistry::engineRoot() const
	{
		return mEngineRootPath;
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

		if (!exists(assetCachePath))
		{
			saveAssetCache();
		}

		std::ifstream is(assetCachePath);

		json data;
		is >> data;

		// Load AssetCache from file
		AssetCache assetCache = data;

		// Create Assets from factories based on their stored type
		for (const auto& [id, assetCacheData] : assetCache.data)
		{
			std::string path = assetCacheData.path;
			std::string type = assetCacheData.type;
			std::shared_ptr<Asset> asset;

			for (const auto& factory : mAssetFactories)
			{
				if (factory->type() == type)
				{
					asset = factory->addAsset(id, path);
					break;
				}
			}

			mIdToAssetMap[asset->id()] = asset;
			mPathToIdMap[asset->relativePath().string()] = asset->id();
		}
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::getAsset(const PuffinID& uuid)
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

	fs::path AssetRegistry::findEngineRoot(const fs::path& currentPath)
	{
		bool cmakeListsInDir = false;
		fs::path localPath = currentPath;

		while (!cmakeListsInDir)
		{
			for (const auto& entry : fs::recursive_directory_iterator(localPath))
			{
				const auto& entryPath = entry.path();

				if (strcmp(entryPath.filename().string().c_str(), "PuffinEngine.txt") == 0)
				{
					cmakeListsInDir = true;
					localPath = entryPath.parent_path();
					break;
				}
			}

			if (!cmakeListsInDir)
			{
				localPath = localPath.parent_path();
			}
		}

		return localPath;
	}
}

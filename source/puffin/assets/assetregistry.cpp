#include "puffin/assets/assetregistry.h"

#include <fstream>

namespace puffin::assets
{
	AssetRegistry* AssetRegistry::sInstance = nullptr;

	void AssetRegistry::Initialize(const io::ProjectFile& projectFile, const fs::path& projectPath)
	{
		mProjectName = projectFile.name;
		mProjectRootPath = projectPath;
		mProjectRootPath.remove_filename();

		mEngineRootPath = FindEngineRoot(fs::current_path());

		mContentDirectories.push_back(mProjectRootPath / "content");

		for (auto& dir : projectFile.additionalContentDirectories)
		{
			mContentDirectories.emplace_back(dir);
		}
	}

	void AssetRegistry::SetProjectName(const std::string& projectName)
	{
		mProjectName = projectName;
	}

	const std::string& AssetRegistry::GetProjectName()
	{
		return mProjectName;
	}

	void AssetRegistry::SetProjectRoot(fs::path projectRootPath)
	{
		mProjectRootPath = projectRootPath;
	}

	fs::path AssetRegistry::GetProjectRoot()
	{
		return mProjectRootPath;
	}

	fs::path AssetRegistry::GetContentRoot() const
	{
		return mContentDirectories[0];
	}

	fs::path AssetRegistry::GetEngineRoot() const
	{
		return mEngineRootPath;
	}

	void AssetRegistry::SaveAssetCache() const
	{
		// Fill AssetCache Struct
		AssetCache assetCache;

		for (const auto& [fst, snd] : mIdToAssetMap)
		{
			assetCache.data[fst].path = snd->GetRelativePath().string();
			assetCache.data[fst].type = snd->GetType();
		}

		json data = assetCache;

		// Initialize Output File Stream and Cereal Binary Archive
		const fs::path assetCachePath = mProjectRootPath / (mProjectName + ".passetcache");
		std::ofstream os(assetCachePath, std::ios::out);

		os << std::setw(4) << data << std::endl;

		os.close();
	}

	void AssetRegistry::LoadAssetCache()
	{
		// Initialize Input File Stream and Cereal Binary Archive
		const fs::path assetCachePath = mProjectRootPath / (mProjectName + ".passetcache");

		if (!exists(assetCachePath))
		{
			SaveAssetCache();
		}

		std::ifstream is(assetCachePath);

		json data;
		is >> data;

		// Load AssetCache from file
		AssetCache assetCache = data;

		// Create Assets from factories based on their stored type
		for (const auto& [id, assetCacheData] : assetCache.data)
		{
			auto path = fs::path(assetCacheData.path).make_preferred();
			std::string type = assetCacheData.type;
			std::shared_ptr<Asset> asset;

			for (const auto& factory : mAssetFactories)
			{
				if (factory->GetType() == type)
				{
					asset = factory->AddAsset(id, path);
					break;
				}
			}

			mIdToAssetMap[asset->GetID()] = asset;
			mPathToIdMap[asset->GetRelativePath().string()] = asset->GetID();
		}
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::GetAsset(const UUID& uuid)
	{
		// Return asset if it has been cached
		if (mIdToAssetMap.count(uuid))
		{
			return mIdToAssetMap[uuid];
		}

		// No asset with that ID has been registered, return nullptr
		return nullptr;
	}

	std::shared_ptr<Asset> AssetRegistry::GetAsset(const fs::path& path)
	{
		// Return asset if it has been cached
		if (mPathToIdMap.count(path.string()))
		{
			return GetAsset(mPathToIdMap[path.string()]);
		}

		// No asset with that path has been registered, return nullptr
		return nullptr;
	}

	fs::path AssetRegistry::FindEngineRoot(const fs::path& currentPath)
	{
		bool cmakeListsInDir = false;
		fs::path localPath = currentPath;

		while (!cmakeListsInDir)
		{
			for (const auto& entry : fs::recursive_directory_iterator(localPath))
			{
				if (const auto& entryPath = entry.path(); strcmp(entryPath.filename().string().c_str(), "PuffinEngine.txt") == 0)
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

	std::shared_ptr<Asset> AssetRegistry::FindAssetAtPath(const fs::path& path)
	{
		for (const auto& contentDir : mContentDirectories)
		{
			if (const auto fullPath = contentDir / path; fs::exists(fullPath))
			{
				AssetData assetData;

				if (mJsonFileTypes.find(fullPath.extension().string()) != mJsonFileTypes.end())
				{
					LoadJsonFile(fullPath, assetData);
				}
				else
				{
					LoadBinaryFile(fullPath, assetData, true);
				}

				const auto& assetType = gAssetTypeToString.at(assetData.type);
				std::shared_ptr<Asset> asset;

				for (const auto& factory : mAssetFactories)
				{
					if (factory->GetType() == assetType)
					{
						return factory->AddAsset(assetData.id, path);
					}
				}
			}
		}

		return nullptr;
	}
}

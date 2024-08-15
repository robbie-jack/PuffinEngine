#pragma once

#include "puffin/assets/asset.h"
#include "puffin/projectsettings.h"
#include "nlohmann/json.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <set>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin::assets
{
	/*
	 * Asset Cache
	 * Struct which Asset ID/Path, ID/Type pairs are stored in when saving/loading
	 */
	struct AssetCacheData
	{
		std::string path;
		std::string type;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetCacheData, path, type)
	};

	struct AssetCache
	{
		std::unordered_map<PuffinID, AssetCacheData> data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetCache, data)
	};

	/* Asset Factory Interface
	 * Virtual Interface for templated AssetFactories
	 */
	class IAssetFactory
	{
	public:
		virtual ~IAssetFactory() = default;
		virtual std::string GetType() = 0;
		virtual std::shared_ptr<Asset> AddAsset(PuffinID id, fs::path path) = 0;
	};

	/*
	 * Asset Factory
	 * Template Class which is used for instantiating Assets at runtime
	 */
	template<typename AssetType>
	class AssetFactory final : public IAssetFactory
	{
	public:

		AssetFactory()
		{
			AssetType* asset = new AssetType();
			mAssetTypeString = asset->GetType();
			delete asset;
			asset = nullptr;
		}

		std::string GetType() override
		{
			return mAssetTypeString;
		}

		std::shared_ptr<Asset> AddAsset(PuffinID id, fs::path path) override
		{
			return std::make_shared<AssetType>(id, path);
		}

	private:

		std::string mAssetTypeString;

	};

	/*
	* Asset Registry tracks all registered assets for the current project
	*/
	class AssetRegistry
	{
		static AssetRegistry* sInstance;

		AssetRegistry()
		{
			mJsonFileTypes.insert(".pmaterialinst");
			mJsonFileTypes.insert(".pshader");
		}

	public:

		static AssetRegistry* Get()
		{
			if (!sInstance)
				sInstance = new AssetRegistry();

			return sInstance;
		}

		static void Clear()
		{
			delete sInstance;
			sInstance = nullptr;
		}

		~AssetRegistry()
		{
			for (const auto& [fst, snd] : mIdToAssetMap)
			{
				snd->Unload();
			}

			mIdToAssetMap.clear();
			mPathToIdMap.clear();
			mAssetFactories.clear();
		}


		void Initialize(const io::ProjectFile& projectFile, const fs::path& projectPath);

		void SetProjectName(const std::string& projectName);
		[[nodiscard]] const std::string& GetProjectName();

		void SetProjectRoot(fs::path projectRootPath);
		[[nodiscard]] fs::path GetProjectRoot();
		[[nodiscard]] fs::path GetContentRoot() const;
		[[nodiscard]] fs::path GetEngineRoot() const;

		// Asset Cache Saving/Loading
		void SaveAssetCache() const;
		void LoadAssetCache();

		// Get Asset from Registry
		std::shared_ptr<Asset> GetAsset(const PuffinID& uuid);
		std::shared_ptr<Asset> GetAsset(const fs::path& path);

		// Get Typed Asset from Registry (prefer these over the non typed get asset as these ones do proper type checking)
		template<typename AssetType>
		std::shared_ptr<AssetType> GetAsset(const PuffinID& uuid)
		{
			const auto asset = GetAsset(uuid);

			if (!asset)
			{
				return nullptr;
			}

			std::string type;

			GetAssetType<AssetType>(type);

			if (asset->GetType() != type)
			{
				return nullptr;
			}

			return std::static_pointer_cast<AssetType>(asset);
		}

		template<typename AssetType>
		std::shared_ptr<AssetType> GetAsset(const fs::path& path)
		{
			auto asset = GetAsset(path);

			// Try and find asset by path if it's not stored in 
			if (!asset)
			{
				asset = FindAssetAtPath(path);

				if (!asset)
				{
					return nullptr;
				}
				else
				{
					mIdToAssetMap.emplace(asset->GetID(), asset);
					mPathToIdMap.emplace(asset->GetRelativePath().string(), asset->GetID());
				}
			}

			std::string type;

			GetAssetType<AssetType>(type);

			if (asset->GetType() != type)
			{
				return nullptr;
			}

			return std::static_pointer_cast<AssetType>(asset);
		}

		// Register new Asset to Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> AddAsset(const fs::path& path)
		{
			// First check if there is already an asset using this path
			std::shared_ptr<AssetType> asset = GetAsset<AssetType>(path);
			if (asset == nullptr)
			{
				// If there isn't create a new asset
				asset = std::make_shared<AssetType>(path);

				mIdToAssetMap.emplace(asset->GetID(), asset);
				mPathToIdMap.emplace(asset->GetRelativePath().string(), asset->GetID());
			}

			// Return existing/created asset
			return asset;
		}

		template<typename AssetType>
		void RegisterAssetType()
		{
			std::shared_ptr<AssetFactory<AssetType>> factory = std::make_shared<AssetFactory<AssetType>>();

			mAssetFactories.push_back(factory);
		}

		template<typename AssetType>
		static void GetAssetType(std::string& type)
		{
			const Asset* tempAsset = new AssetType();
			type = tempAsset->GetType();

			delete tempAsset;
			tempAsset = nullptr;
		}

		static fs::path FindEngineRoot(const fs::path& currentPath);

	private:

		std::string mProjectName;
		fs::path mProjectRootPath;
		fs::path mEngineRootPath;

		std::vector<fs::path> mContentDirectories = {};

		// Map of ID's to Asset, generated at runtime
		std::unordered_map<PuffinID, std::shared_ptr<Asset>> mIdToAssetMap;
		std::unordered_map<std::string, PuffinID> mPathToIdMap;

		// Assets with this file type are stored in json instead of binary
		std::set<std::string> mJsonFileTypes;

		// Asset Factories for creating assets at runtime
		std::vector<std::shared_ptr<IAssetFactory>> mAssetFactories;

		template<typename AssetType>
		std::shared_ptr<AssetType> FindAssetAtPath(const fs::path path)
		{
			return std::static_pointer_cast<AssetType>(FindAssetAtPath(path));
		}

		std::shared_ptr<Asset> FindAssetAtPath(const fs::path& path);
	};
}
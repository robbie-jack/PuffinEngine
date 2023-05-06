#pragma once

#include "Asset.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

#include "nlohmann/json.hpp"

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
		std::unordered_map<PuffinId, AssetCacheData> data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetCache, data)
	};

	/* Asset Factory Interface
	 * Virtual Interface for templated AssetFactories
	 */
	class IAssetFactory
	{
	public:
		virtual ~IAssetFactory() = default;
		virtual std::string type() = 0;
		virtual std::shared_ptr<Asset> addAsset(PuffinId id, fs::path path) = 0;
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
			mAssetTypeString = asset->type();
			delete asset;
			asset = nullptr;
		}

		std::string type() override
		{
			return mAssetTypeString;
		}

		std::shared_ptr<Asset> addAsset(PuffinId id, fs::path path) override
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

		AssetRegistry() = default;

	public:

		static AssetRegistry* get()
		{
			if (!sInstance)
				sInstance = new AssetRegistry();

			return sInstance;
		}

		static void clear()
		{
			delete sInstance;
			sInstance = nullptr;
		}

		~AssetRegistry()
		{
			for (const auto& [fst, snd] : mIdToAssetMap)
			{
				snd->unload();
			}

			mIdToAssetMap.clear();
			mPathToIdMap.clear();
			mAssetFactories.clear();
		}

		void setProjectName(const std::string& projectName);
		std::string projectName();

		void setProjectRoot(fs::path projectRootPath);
		fs::path setProjectRoot();

		fs::path contentRoot() const;

		// Asset Cache Saving/Loading
		void saveAssetCache() const;
		void loadAssetCache();

		// Get Asset from Registry
		std::shared_ptr<Asset> getAsset(const PuffinId& uuid);

		std::shared_ptr<Asset> getAsset(const fs::path& path);

		// Get Typed Asset from Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> getAsset(const PuffinId& uuid)
		{
			return std::static_pointer_cast<AssetType>(getAsset(uuid));
		}

		template<typename AssetType>
		std::shared_ptr<AssetType> getAsset(const fs::path& path)
		{
			return std::static_pointer_cast<AssetType>(getAsset(path));
		}

		// Register new Asset to Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> addAsset(const fs::path& path)
		{
			// First check if there is already an asset using this path
			std::shared_ptr<AssetType> asset = getAsset<AssetType>(path);
			if (asset == nullptr)
			{
				// If there isn't create a new asset
				asset = std::make_shared<AssetType>(path);

				mIdToAssetMap.emplace(asset->id(), asset);
				mPathToIdMap.emplace(asset->relativePath().string(), asset->id());
			}

			// Return existing/created asset
			return asset;
		}

		template<typename AssetType>
		void registerAssetType()
		{
			std::shared_ptr<AssetFactory<AssetType>> factory = std::make_shared<AssetFactory<AssetType>>();

			mAssetFactories.push_back(factory);
		}

	private:

		std::string mProjectName;
		fs::path mProjectRootPath;

		// Map of ID's to Asset, generated at runtime
		std::unordered_map<PuffinId, std::shared_ptr<Asset>> mIdToAssetMap;
		std::unordered_map<std::string, PuffinId> mPathToIdMap;

		// Asset Factories for creating assets at runtime
		std::vector<std::shared_ptr<IAssetFactory>> mAssetFactories;
	};
}
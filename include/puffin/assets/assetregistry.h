#pragma once

#include "puffin/assets/asset.h"
#include "puffin/project_settings.h"
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
		virtual std::string type() = 0;
		virtual std::shared_ptr<Asset> add_asset(PuffinID id, fs::path path) = 0;
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
			m_asset_type_string = asset->type();
			delete asset;
			asset = nullptr;
		}

		std::string type() override
		{
			return m_asset_type_string;
		}

		std::shared_ptr<Asset> add_asset(PuffinID id, fs::path path) override
		{
			return std::make_shared<AssetType>(id, path);
		}

	private:

		std::string m_asset_type_string;

	};

	/*
	* Asset Registry tracks all registered assets for the current project
	*/
	class AssetRegistry
	{
		static AssetRegistry* sInstance;

		AssetRegistry()
		{
			m_json_file_types.insert(".pmaterialinst");
			m_json_file_types.insert(".pshader");
		}

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
			for (const auto& [fst, snd] : m_id_to_asset_map)
			{
				snd->unload();
			}

			m_id_to_asset_map.clear();
			m_path_to_id_map.clear();
			m_asset_factories.clear();
		}


		void init(const io::ProjectFile& projectFile, const fs::path& projectPath);

		void set_project_name(const std::string& projectName);
		std::string project_name();

		void set_project_root(fs::path projectRootPath);
		fs::path project_root();

		fs::path content_root() const;

		fs::path engine_root() const;

		// Asset Cache Saving/Loading
		void save_asset_cache() const;
		void load_asset_cache();

		// Get Asset from Registry
		std::shared_ptr<Asset> get_asset(const PuffinID& uuid);

		std::shared_ptr<Asset> get_asset(const fs::path& path);

		// Get Typed Asset from Registry (prefer these over the non typed get asset as these ones do proper type checking)
		template<typename AssetType>
		std::shared_ptr<AssetType> get_asset(const PuffinID& uuid)
		{
			const auto asset = get_asset(uuid);

			if (!asset)
			{
				return nullptr;
			}

			std::string type;

			get_asset_type<AssetType>(type);

			if (asset->type() != type)
			{
				return nullptr;
			}

			return std::static_pointer_cast<AssetType>(asset);
		}

		template<typename AssetType>
		std::shared_ptr<AssetType> get_asset(const fs::path& path)
		{
			auto asset = get_asset(path);

			// Try and find asset by path if it's not stored in 
			if (!asset)
			{
				asset = find_asset_at_path(path);

				if (!asset)
				{
					return nullptr;
				}
				else
				{
					m_id_to_asset_map.emplace(asset->id(), asset);
					m_path_to_id_map.emplace(asset->relativePath().string(), asset->id());
				}
			}

			std::string type;

			get_asset_type<AssetType>(type);

			if (asset->type() != type)
			{
				return nullptr;
			}

			return std::static_pointer_cast<AssetType>(asset);
		}

		// Register new Asset to Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> add_asset(const fs::path& path)
		{
			// First check if there is already an asset using this path
			std::shared_ptr<AssetType> asset = get_asset<AssetType>(path);
			if (asset == nullptr)
			{
				// If there isn't create a new asset
				asset = std::make_shared<AssetType>(path);

				m_id_to_asset_map.emplace(asset->id(), asset);
				m_path_to_id_map.emplace(asset->relativePath().string(), asset->id());
			}

			// Return existing/created asset
			return asset;
		}

		template<typename AssetType>
		void register_asset_type()
		{
			std::shared_ptr<AssetFactory<AssetType>> factory = std::make_shared<AssetFactory<AssetType>>();

			m_asset_factories.push_back(factory);
		}

		template<typename AssetType>
		static void get_asset_type(std::string& type)
		{
			const Asset* tempAsset = new AssetType();
			type = tempAsset->type();

			delete tempAsset;
			tempAsset = nullptr;
		}

		static fs::path find_engine_root(const fs::path& currentPath);

	private:

		std::string m_project_name;
		fs::path m_project_root_path;
		fs::path m_engine_root_path;

		std::vector<fs::path> m_content_directories = {};

		// Map of ID's to Asset, generated at runtime
		std::unordered_map<PuffinID, std::shared_ptr<Asset>> m_id_to_asset_map;
		std::unordered_map<std::string, PuffinID> m_path_to_id_map;

		// Assets with this file type are stored in json instead of binary
		std::set<std::string> m_json_file_types;

		// Asset Factories for creating assets at runtime
		std::vector<std::shared_ptr<IAssetFactory>> m_asset_factories;

		template<typename AssetType>
		std::shared_ptr<AssetType> find_asset_at_path(fs::path path)
		{
			return std::static_pointer_cast<AssetType>(find_asset_at_path(path));
		}

		std::shared_ptr<Asset> find_asset_at_path(fs::path path);
	};
}
#include "puffin/assets/assetregistry.h"

#include <fstream>

namespace puffin::assets
{
	AssetRegistry* AssetRegistry::sInstance = nullptr;

	void AssetRegistry::init(const io::ProjectFile& projectFile, const fs::path& projectPath)
	{
		m_project_name = projectFile.name;
		m_project_root_path = projectPath;
		m_project_root_path.remove_filename();

		m_engine_root_path = find_engine_root(fs::current_path());

		m_content_directories.push_back(m_project_root_path / "content");

		for (auto& dir : projectFile.additional_content_directories)
		{
			m_content_directories.emplace_back(dir);
		}
	}

	void AssetRegistry::set_project_name(const std::string& projectName)
	{
		m_project_name = projectName;
	}

	std::string AssetRegistry::project_name()
	{
		return m_project_name;
	}

	void AssetRegistry::set_project_root(fs::path projectRootPath)
	{
		m_project_root_path = projectRootPath;
	}

	fs::path AssetRegistry::project_root()
	{
		return m_project_root_path;
	}

	fs::path AssetRegistry::content_root() const
	{
		return m_content_directories[0];
	}

	fs::path AssetRegistry::engine_root() const
	{
		return m_engine_root_path;
	}

	void AssetRegistry::save_asset_cache() const
	{
		// Fill AssetCache Struct
		AssetCache assetCache;

		for (const auto& [fst, snd] : m_id_to_asset_map)
		{
			assetCache.data[fst].path = snd->GetRelativePath().string();
			assetCache.data[fst].type = snd->GetType();
		}

		json data = assetCache;

		// Initialize Output File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_project_root_path / (m_project_name + ".passetcache");
		std::ofstream os(assetCachePath, std::ios::out);

		os << std::setw(4) << data << std::endl;

		os.close();
	}

	void AssetRegistry::load_asset_cache()
	{
		// Initialize Input File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_project_root_path / (m_project_name + ".passetcache");

		if (!exists(assetCachePath))
		{
			save_asset_cache();
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

			for (const auto& factory : m_asset_factories)
			{
				if (factory->type() == type)
				{
					asset = factory->add_asset(id, path);
					break;
				}
			}

			m_id_to_asset_map[asset->GetID()] = asset;
			m_path_to_id_map[asset->GetRelativePath().string()] = asset->GetID();
		}
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::get_asset(const PuffinID& uuid)
	{
		// Return asset if it has been cached
		if (m_id_to_asset_map.count(uuid))
		{
			return m_id_to_asset_map[uuid];
		}

		// No asset with that ID has been registered, return nullptr
		return nullptr;
	}

	std::shared_ptr<Asset> AssetRegistry::get_asset(const fs::path& path)
	{
		// Return asset if it has been cached
		if (m_path_to_id_map.count(path.string()))
		{
			return get_asset(m_path_to_id_map[path.string()]);
		}

		// No asset with that path has been registered, return nullptr
		return nullptr;
	}

	fs::path AssetRegistry::find_engine_root(const fs::path& currentPath)
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

	std::shared_ptr<Asset> AssetRegistry::find_asset_at_path(fs::path path)
	{
		for (const auto& content_dir : m_content_directories)
		{
			if (const auto full_path = content_dir / path; fs::exists(full_path))
			{
				AssetData asset_data;

				if (m_json_file_types.find(full_path.extension().string()) != m_json_file_types.end())
				{
					LoadJsonFile(full_path, asset_data);
				}
				else
				{
					LoadBinaryFile(full_path, asset_data, true);
				}

				const auto& asset_type = gAssetTypeToString.at(asset_data.type);
				std::shared_ptr<Asset> asset;

				for (const auto& factory : m_asset_factories)
				{
					if (factory->type() == asset_type)
					{
						return factory->add_asset(asset_data.ID, path);
					}
				}
			}

			return nullptr;
		}
	}
}

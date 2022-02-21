#pragma once

#include "Types/UUID.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin::Assets
{
	struct AssetData
	{
		std::string type;
		uint32_t version;
		std::string json;
		std::vector<char> binaryBlob;
	};

	static bool SaveBinaryFile(const char* path, const AssetData& data);

	static bool LoadBinaryFile(const char* path, const AssetData& data);

	class Asset
	{
	public:

		Asset(const fs::path& path) : m_path(path) {}
		Asset(const UUID uuid, const fs::path& path) : m_id(uuid), m_path(path) {}

		UUID ID()
		{
			return m_id;
		}

		fs::path RelativePath()
		{
			return m_path;
		}

		void RelativePath(fs::path path)
		{
			m_path = path;
		}

		virtual const std::string& Type() const = 0;
		virtual const uint32_t& Version() const = 0;

	protected:

		bool m_isLoaded = false; // Is Asset Currently Loaded

	private:

		UUID m_id; // UUID of Asset
		fs::path m_path; // Relative Asset Path

	};
}
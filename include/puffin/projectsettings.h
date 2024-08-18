#pragma once

#include <fstream>
#include <string>
#include <filesystem>

#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin
{
	namespace io
	{
		// IO File Structs
		struct ProjectFile
		{
			std::string name;
			std::string defaultScenePath;
			std::vector<std::string> additionalContentDirectories = {};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectFile, name, defaultScenePath, additionalContentDirectories)
		};

		// IO Static Functions
		static void SaveProject(const fs::path& filePath, const ProjectFile& file)
		{
			const json data = file;

			std::ofstream os(filePath.string());

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		static void LoadProject(const fs::path& filePath, ProjectFile& file)
		{
			if (!exists(filePath))
				return;

			std::ifstream is(filePath.string());

			json data;
			is >> data;

			is.close();

			file = data;
		}
	}
}
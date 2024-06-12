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
			std::string default_scene_path;
			std::vector<std::string> additional_content_directories = {};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectFile, name, default_scene_path, additional_content_directories)
		};

		// IO Static Functions
		static void save_project(fs::path file_path, const ProjectFile& file)
		{
			const json data = file;

			std::ofstream os(file_path.string());

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		static void load_project(fs::path file_path, ProjectFile& file)
		{
			if (!exists(file_path))
				return;

			std::ifstream is(file_path.string());

			json data;
			is >> data;

			is.close();

			file = data;
		}
	}
}
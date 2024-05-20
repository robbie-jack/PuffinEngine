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

		struct ProjectSettings
		{
			float mouse_sensitivity = 0.05f;
			float camera_fov = 60.f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectSettings, mouse_sensitivity, camera_fov)
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
		
		static void save_settings(fs::path file_path, const ProjectSettings& settings)
		{
			const json data = settings;

			std::ofstream os(file_path.string());

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		static void load_settings(fs::path file_path, ProjectSettings& settings)
		{
			if (!fs::exists(file_path))
				return;

			std::ifstream is(file_path.string());

			json data;
			is >> data;

			is.close();

			settings = data;
		}
	}
}
#pragma once

#include <fstream>
#include <string>
#include <filesystem>

#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin
{
	namespace IO
	{
		// IO File Structs
		struct ProjectFile
		{
			std::string name;
			std::string defaultScenePath;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectFile, name, defaultScenePath)
		};

		struct ProjectSettings
		{
			float mouseSensitivity = 0.05f;
			float cameraFov = 60.f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectSettings, mouseSensitivity, cameraFov)
		};

		// IO Static Functions
		static void SaveProject(fs::path file_path, const ProjectFile& file)
		{
			const json data = file;

			std::ofstream os(file_path.string());

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		static void LoadProject(fs::path file_path, ProjectFile& file)
		{
			if (!fs::exists(file_path))
				return;

			std::ifstream is(file_path.string());

			json data;
			is >> data;

			is.close();

			file = data;
		}
		
		static void SaveSettings(fs::path file_path, const ProjectSettings& settings)
		{
			const json data = settings;

			std::ofstream os(file_path.string());

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		static void LoadSettings(fs::path file_path, ProjectSettings& settings)
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
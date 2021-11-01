#pragma once

#ifndef PROJECT_SETTINGS_H
#define PROJECT_SETTINGS_H

#include <fstream>
#include <string>
#include <cereal/archives/json.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin
{
	namespace IO
	{
		// IO File Structs
		struct ProjectFile
		{
			std::string name;
			std::string defaultScenePath;
		};

		template<class Archive>
		void serialize(Archive& archive, ProjectFile& file)
		{
			archive(file.name);
			archive(file.defaultScenePath);
		}

		struct ProjectSettings
		{
			float mouseSensitivity;
			float cameraFov;
		};

		template<class Archive>
		void serialize(Archive& archive, ProjectSettings& settings)
		{
			archive(CEREAL_NVP(settings.cameraFov));
			archive(CEREAL_NVP(settings.mouseSensitivity));
		}

		// IO Static Functions
		static void SaveProject(fs::path file_path, const ProjectFile& file)
		{
			std::ofstream os(file_path);
			cereal::JSONOutputArchive archive(os);

			archive(file);
		}

		static void LoadProject(fs::path file_path, ProjectFile& file)
		{
			if (!fs::exists(file_path))
				return;

			std::ifstream is(file_path);
			cereal::JSONInputArchive archive(is);

			archive(file);
		}
		
		static void SaveSettings(fs::path file_path, const ProjectSettings& settings)
		{
			std::ofstream os(file_path);
			cereal::JSONOutputArchive archive(os);

			archive(settings);
		}

		static void LoadSettings(fs::path file_path, ProjectSettings& settings)
		{
			std::ifstream is(file_path);
			cereal::JSONInputArchive archive(is);

			archive(settings);
		}
	}
}

#endif // PROJECT_SETTINGS_H
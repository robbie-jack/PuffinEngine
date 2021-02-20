#pragma once

#ifndef PROJECT_SETTINGS_H
#define PROHECT_SETTINGS_H

#include <fstream>
#include <string>
#include <cereal/archives/xml.hpp>

namespace Puffin
{
	struct ProjectSettings
	{
		std::string projectName;
		std::string projectDir;
		float mouseSensitivity;
		float cameraFov;
	};

	template<class Archive>
	void serialize(Archive& archive, Puffin::ProjectSettings& settings)
	{
		archive(CEREAL_NVP(settings.projectName));
		archive(CEREAL_NVP(settings.projectDir));
		archive(CEREAL_NVP(settings.cameraFov));
		archive(CEREAL_NVP(settings.mouseSensitivity));
	}
}

namespace Puffin
{
	namespace IO
	{
		static void SaveSettings(std::string file_name, const ProjectSettings& settings)
		{
			std::ofstream os(file_name);
			cereal::XMLOutputArchive archive(os);

			archive(settings);
		}

		static void LoadSettings(std::string file_name, ProjectSettings& settings)
		{
			std::ifstream is(file_name);
			cereal::XMLInputArchive archive(is);

			archive(settings);
		}
	}
}

#endif // PROJECT_SETTINGS_H
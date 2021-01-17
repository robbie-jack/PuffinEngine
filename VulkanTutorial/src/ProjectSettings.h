#pragma once

#ifndef PROJECT_SETTINGS_H
#define PROHECT_SETTINGS_H

#include <fstream>
#include <cereal/archives/xml.hpp>

namespace Puffin
{
	struct ProjectSettings
	{
		float mouseSensitivity;
		float cameraFov;
	};

	template<class Archive>
	void serialize(Archive& archive, Puffin::ProjectSettings& settings)
	{
		archive(CEREAL_NVP(settings.cameraFov),
			CEREAL_NVP(settings.mouseSensitivity));
	}
}

namespace Puffin
{
	namespace IO
	{
		static void SaveSettings(std::string file_name, ProjectSettings settings)
		{
			std::ofstream os(file_name);
			cereal::XMLOutputArchive archive(os);

			archive(settings);
		}

		static ProjectSettings LoadSettings(std::string file_name)
		{
			std::ifstream is(file_name);
			cereal::XMLInputArchive archive(is);

			ProjectSettings settings;
			archive(settings);

			return settings;
		}
	}
}

#endif // PROJECT_SETTINGS_H
#include "puffin/core/settingsmanager.h"

#include <iostream>

#include "puffin/core/engine.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/types/vector3.h"

namespace puffin::core
{
	SettingsManager::SettingsManager(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		mName = "SettingsManager";
	}

	void SettingsManager::Initialize(core::SubsystemManager* subsystemManager)
	{
		if (mEngine->GetSetupEngineDefaultSettings())
		{
			default_settings();
			save(assets::AssetRegistry::Get()->GetProjectRoot() / "config" / "settings.json");
		}
		else
		{
			load(assets::AssetRegistry::Get()->GetProjectRoot() / "config" / "settings.json");
		}
	}

	void SettingsManager::load(const fs::path& path)
	{
		if (!fs::exists(path))
			return;

		std::ifstream is(path.string());
		is >> m_json;

		is.close();
	}

	void SettingsManager::save(const fs::path& path)
	{
		if (!exists(path.parent_path()))
			create_directory(path.parent_path());

		std::ofstream os(path.string());

		os << std::setw(4) << m_json << std::endl;

		os.close();
	}

	void SettingsManager::default_settings()
	{
		Set("editor_camera_fov", 60.0f);
		Set("mouse_sensitivity", 0.05f);
		Set("unit_scale", 1.0f);

		Set("rendering_draw_shadows", false);

		Set("physics_gravity", Vector3f(0.0f, -9.81f, 0.0f));
	}
}

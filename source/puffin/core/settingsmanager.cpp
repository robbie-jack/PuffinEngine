#include "puffin/core/settings_manager.h"

#include <iostream>

#include "puffin/core/engine.h"
#include "puffin/assets/asset_registry.h"
#include "puffin/types/vector.h"

namespace puffin::core
{
	SettingsManager::SettingsManager(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		m_name = "SettingsManager";
	}

	void SettingsManager::initialize(core::SubsystemManager* subsystem_manager)
	{
		if (m_engine->setup_engine_default_settings())
		{
			default_settings();
			save(assets::AssetRegistry::get()->project_root() / "config" / "settings.json");
		}
		else
		{
			load(assets::AssetRegistry::get()->project_root() / "config" / "settings.json");
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
		set("editor_camera_fov", 60.0f);
		set("mouse_sensitivity", 0.05f);
		set("unit_scale", 1.0f);

		set("rendering_draw_shadows", false);

		set("physics_gravity", Vector3f(0.0f, -9.81f, 0.0f));
	}
}

#include "puffin/core/settingsmanager.h"

#include <utility>

#include "puffin/core/engine.h"
#include "puffin/assets/assetregistry.h"

namespace puffin::core
{
	SettingsCategory::SettingsCategory(std::shared_ptr<Engine> engine, std::string name)
		: mEngine(std::move(engine)), mName(std::move(name))
	{
	}

	void SettingsCategory::SetData(const toml::table& data)
	{
		mData = data;

		const auto signalSubsystem = mEngine->GetSubsystem<SignalSubsystem>();
		
		for (const auto& [key, value] : mData)
		{
			if (std::string signalName = mName + "_" + std::string(key.str()); !signalSubsystem->GetSignal(signalName))
			{
				signalSubsystem->CreateSignal(signalName);
			}
		}
	}

	const toml::table& SettingsCategory::GetData() const
	{
		return mData;
	}

	SettingsManager::SettingsManager(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		mName = "SettingsManager";
	}

	void SettingsManager::Initialize(core::SubsystemManager* subsystemManager)
	{
		mCategories.emplace("general", SettingsCategory(mEngine, "general"));
		mCategories.emplace("editor", SettingsCategory(mEngine, "editor"));
		mCategories.emplace("physics", SettingsCategory(mEngine, "physics"));
		mCategories.emplace("rendering", SettingsCategory(mEngine, "rendering"));
		
		if (mEngine->GetSetupEngineDefaultSettings())
		{
			DefaultSettings();
			Save(assets::AssetRegistry::Get()->GetProjectRoot() / "config" / "settings.toml");
		}
		else
		{
			Load(assets::AssetRegistry::Get()->GetProjectRoot() / "config" / "settings.toml");
		}
	}

	SettingsCategory& SettingsManager::GetCategory(const std::string& name)
	{
		if (mCategories.find(name) == mCategories.end())
		{
			mCategories.emplace(name, SettingsCategory(mEngine, name));
		}

		return mCategories[name];
	}

	void SettingsManager::Load(const fs::path& path)
	{
		if (!fs::exists(path))
			return;

		std::ifstream is(path.string());

		auto tbl = toml::parse(is);

		for (auto& [ name, category ] : mCategories)
		{
			if (tbl.contains(name))
			{
				category.SetData(*tbl.at(name).as_table());
			}
		}

		is.close();
	}

	void SettingsManager::Save(const fs::path& path)
	{
		toml::table tbl;

		for (auto& [ name, category ] : mCategories)
		{
			tbl.emplace(name, category.GetData());
		}

		if (!exists(path.parent_path()))
			create_directory(path.parent_path());
		
		std::ofstream os(path.string());

		os << tbl << "\n\n";

		os.close();
	}

	void SettingsManager::DefaultSettings()
	{
		// General
		{
			auto& generalSettings = mCategories["general"];

			generalSettings.Set("framerate_limit_enable", true);
			generalSettings.Set("framerate_limit", 60);
			generalSettings.Set("unit_scale", 1.0);
			generalSettings.Set("mouse_sensitivity", 0.05);
		}

		// Editor
		{
			auto& editorSettings = mCategories["editor"];

			editorSettings.Set("camera_fov", 60.0);
		}

		// Physics
		{
			auto& physicsSettings = mCategories["physics"];

			physicsSettings.Set("ticks_per_second", 60);
			physicsSettings.Set("sub_steps", 4);
			physicsSettings.Set("gravity_x", 0.0);
			physicsSettings.Set("gravity_y", -9.81);
			physicsSettings.Set("gravity_z", 0.0);
			physicsSettings.Set("box2d_enable", true);
			physicsSettings.Set("jolt_enable", false);
		}

		// Rendering
		{
			auto& renderingSettings = mCategories["rendering"];

			renderingSettings.Set("physics_interpolation_enable", true);
			renderingSettings.Set("shadows_enable", true);
		}
	}
}

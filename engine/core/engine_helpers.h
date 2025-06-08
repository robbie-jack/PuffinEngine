#pragma once

#include <memory>
#include <filesystem>

#include "core/engine.h"
#include "scene/scene_serialization_subsystem.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace io
	{
		struct ProjectFile;
	}
}

namespace puffin::core
{
	class Engine;

	void RegisterRequiredSubsystems(const std::shared_ptr<Engine>& engine);

	void RegisterComponentTypes2D();
	void RegisterComponentTypes3D();

	void RegisterNodeTypes2D();
	void RegisterNodeTypes3D();

	//void RegisterAssetTypes(const io::ProjectFile& projectFile, const std::filesystem::path& projectPath);

	//void AddDefaultAssets();
	//void ImportDefaultAssets();
	//void LoadAndSaveAssets();

	void SetupDefaultScene2D(const std::shared_ptr<Engine>& engine);
	void SetupDefaultPhysicsScene2D(const std::shared_ptr<Engine>& engine);

	void SetupDefaultScene3D(const std::shared_ptr<Engine>& engine);
	void SetupDefaultPhysicsScene3D(const std::shared_ptr<Engine>& engine);
}

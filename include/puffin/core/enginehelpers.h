#pragma once

#include <memory>
#include <filesystem>

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

	void RegisterAssetTypes(const io::ProjectFile& projectFile, const std::filesystem::path& projectPath);
	void RegisterComponentTypes();
	void RegisterNodeTypes();
	void RegisterComponentsForSerialization(const std::shared_ptr<Engine>& engine);
	void RegisterRequiredSubsystems(const std::shared_ptr<Engine>& engine);

	void AddDefaultAssets();
	void ImportDefaultAssets();
	void LoadAndSaveAssets();

	void SetupDefaultScene(const std::shared_ptr<Engine>& engine);
	void SetupDefaultPhysicsScene3D(const std::shared_ptr<Engine>& engine);
}

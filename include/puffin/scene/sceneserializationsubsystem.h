#pragma once

#include <fstream>
#include <filesystem>
#include <utility>

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"
#include "entt/meta/factory.hpp"

#include "puffin/assets/assetregistry.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/subsystem.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/types/uuid.h"
#include "puffin/utility/serialization.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace puffin::io
{
	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData() = default;
		explicit SceneData(fs::path path);

		// Initialize ECS & SceneGraph with loaded data
		void Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph);

		void UpdateData(const std::shared_ptr<core::Engine>& engine);

		void Clear();

		// Save Entities/Components to json scene file
		void Save();

		// Load  Entities/Components from Binary Scene File
		void Load(const bool forceLoad = false);
		void LoadAndInit(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph);

		void SetPath(const fs::path& path);
		const fs::path& GetPath();

	private:

		using EntityJsonMap = std::unordered_map<UUID, nlohmann::json>;

		struct SerializedNodeData
		{
			UUID id;
			std::string name;
			std::string type;
			std::vector<UUID> childIDs;
			nlohmann::json json;
		};

		fs::path mPath;
		bool mHasData = false; // This scene contains a copy of active scene data, either loaded from file or copied from ecs

		std::vector<UUID> mEntityIDs;
		std::unordered_map<entt::id_type, EntityJsonMap> mEntityJsonMaps;

		std::vector<UUID> mRootNodeIDs;
		std::vector<UUID> mNodeIDs;
		std::unordered_map<UUID, SerializedNodeData> mSerializedNodeData;

		void SerializeNodeAndChildren(scene::SceneGraphSubsystem* sceneGraph, UUID id);
	};

	class SceneSerializationSubsystem : public core::Subsystem
	{
	public:

		explicit SceneSerializationSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneSerializationSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void BeginPlay() override;
		void EndPlay() override;

		void Load() const;
		void Setup() const;
		void LoadAndSetup() const;

		std::shared_ptr<SceneData> CreateScene(const fs::path& path);

		std::shared_ptr<SceneData> GetSceneData();

	private:

		std::shared_ptr<SceneData> mCurrentSceneData = nullptr;
        std::unordered_map<fs::path, std::shared_ptr<SceneData>> mSceneData;

	};
}

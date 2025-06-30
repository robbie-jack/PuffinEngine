#pragma once

#include <fstream>
#include <filesystem>
#include <utility>

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"
#include "entt/meta/factory.hpp"

#include "scene/scene_graph_subsystem.h"
#include "subsystem/engine_subsystem.h"
#include "ecs/entt_subsystem.h"
#include "types/uuid.h"
#include "utility/serialization.h"
#include "scene/scene_info.h"

namespace fs = std::filesystem;

namespace puffin
{
	namespace scene
	{
		// Stores Loaded Scene Data
		class SceneData
		{
		public:

			SceneData() = default;
			explicit SceneData(fs::path path);
			explicit SceneData(fs::path path, SceneInfo sceneInfo);

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

			const SceneInfo& GetSceneInfo() const;

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

			fs::path m_path;
			SceneInfo m_sceneInfo;

			bool m_hasData = false; // This scene contains a copy of active scene data, either loaded from file or copied from ecs

			std::vector<UUID> m_entityIDs;
			std::unordered_map<entt::id_type, EntityJsonMap> m_serializedComponentData;

			std::vector<UUID> m_rootNodeIDs;
			std::vector<UUID> m_nodeIDs;
			std::unordered_map<UUID, SerializedNodeData> m_serializedNodeData;

			void SerializeNodeAndChildren(scene::SceneGraphSubsystem* sceneGraph, UUID id);
		};

		class SceneSerializationSubsystem : public core::EngineSubsystem
		{
		public:

			explicit SceneSerializationSubsystem(const std::shared_ptr<core::Engine>& engine);
			~SceneSerializationSubsystem() override;

			void PreInitialize(core::SubsystemManager* subsystemManager) override;
			void Initialize() override;
			void Deinitialize() override;

			void BeginPlay() override;
			void EndPlay() override;

			std::string_view GetName() const override;

			void Load() const;
			void LoadFromFile(const fs::path& path);
			void Setup() const;
			void LoadAndSetup() const;

			std::shared_ptr<SceneData> CreateScene(const fs::path& path, const SceneInfo& sceneInfo);

			std::shared_ptr<SceneData> GetCurrentSceneData();

		private:

			std::shared_ptr<SceneData> m_currentSceneData = nullptr;
			std::unordered_map<fs::path, std::shared_ptr<SceneData>> m_sceneData;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<scene::SceneSerializationSubsystem>()
		{
			return "SceneSerializationSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<scene::SceneSerializationSubsystem>()
		{
			return entt::hs(GetTypeString<scene::SceneSerializationSubsystem>().data());
		}

		template<>
		inline void RegisterType<scene::SceneSerializationSubsystem>()
		{
			auto meta = entt::meta<scene::SceneSerializationSubsystem>()
				.base<core::EngineSubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}

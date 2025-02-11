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
	class SceneSerializationSubsystem;

	class ISerializableComponent
	{
	public:

		virtual ~ISerializableComponent() = default;

		[[nodiscard]] virtual bool HasComponent(std::shared_ptr<entt::registry> registry, entt::entity entity) const = 0;
		virtual nlohmann::json Serialize(std::shared_ptr<entt::registry> registry, entt::entity entity) = 0;
		virtual void Deserialize(std::shared_ptr<entt::registry> registry, entt::entity entity, const nlohmann::json& json) = 0;
	};

	template<typename T>
	class SerializableComponent : public ISerializableComponent
	{
	public:

		SerializableComponent() = default;
		~SerializableComponent() override = default;

		[[nodiscard]] bool HasComponent(std::shared_ptr<entt::registry> registry, entt::entity entity) const override
		{
			return registry->any_of<T>(entity);
		}

		nlohmann::json Serialize(std::shared_ptr<entt::registry> registry, entt::entity entity) override
		{
			if (registry->any_of<T>(entity))
			{
				const auto& comp = registry->get<T>(entity);
				return serialization::Serialize<T>(comp);
			}

			return nlohmann::json();
		}

		void Deserialize(std::shared_ptr<entt::registry> registry, entt::entity entity, const nlohmann::json& json) override
		{
			auto& comp = registry->emplace_or_replace<T>(entity, serialization::Deserialize<T>(json));
		}
	};

	using SerializableComponentMap = std::unordered_map<entt::id_type, ISerializableComponent*>;

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData() = default;
		explicit SceneData(SceneSerializationSubsystem* sceneSerializationSubsystem, fs::path path);

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

		SceneSerializationSubsystem* mSceneSerializationSubsystem = nullptr;

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

		const SerializableComponentMap& GetSerializableComponents();

		template<typename T>
		void RegisterComponent()
		{
			auto type = entt::resolve<T>();
			auto typeName = type.info().name();

			if (mSerializableComponents.find(type.id()) == mSerializableComponents.end())
			{
				mSerializableComponents.emplace(type.id(), new SerializableComponent<T>());
			}
		}

	private:

		std::shared_ptr<SceneData> mCurrentSceneData = nullptr;
        std::unordered_map<fs::path, std::shared_ptr<SceneData>> mSceneData;
		SerializableComponentMap mSerializableComponents;

	};
}

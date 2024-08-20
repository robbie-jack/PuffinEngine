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

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace puffin::io
{
    // Interface for array to store scene data
	class IComponentDataArray
	{
	public:

		virtual ~IComponentDataArray() = default;

		virtual void Init(ecs::EnTTSubsystem* enttSubsystem) = 0;
		virtual void Update(ecs::EnTTSubsystem* enttSubsystem) = 0;
		virtual void Clear() = 0;
		virtual size_t GetSize() = 0;
		[[nodiscard]] virtual json SaveToJson() const = 0;
		virtual void LoadFromJson(const json& componentData) = 0;
	};

	template<typename CompT>
	class ComponentDataArray : public IComponentDataArray
	{
	public:

		ComponentDataArray() = default;
		~ComponentDataArray() override = default;

		void Init(ecs::EnTTSubsystem* enttSubsystem) override
		{
			const auto registry = enttSubsystem->GetRegistry();

			for (const auto& [id, comp] : mComponents)
			{
				const auto entity = enttSubsystem->GetEntity(id);

				if (registry->any_of<CompT>(entity))
				{
					registry->get<CompT>(entity) = comp;
				}
				else
				{
					registry->emplace_or_replace<CompT>(entity, comp);
				}
			}
		}

		void Update(ecs::EnTTSubsystem* enttSubsystem) override
		{
			Clear();

			const auto registry = enttSubsystem->GetRegistry();

			for (const auto& [ entity, comp ] : registry->view<const CompT>().each())
			{
				const auto& id = enttSubsystem->GetID(entity);

				if (enttSubsystem->ShouldEntityBeSerialized(id))
				{
					mComponents.emplace(id, comp);
				}
			}
		}

		void Clear() override
		{
			mComponents.clear();
		}

		size_t GetSize() override
		{
			return mComponents.size();
		}

		[[nodiscard]] json SaveToJson() const override
		{
			json data;

			int i = 0;
			for (auto& [id, comp] : mComponents)
			{
				data[i] = { id, comp };

				i++;
			}

			return data;
		}

		void LoadFromJson(const json& componentData) override
		{
			for (const auto& data : componentData)
			{
				UUID id = data.at(0);

				mComponents[id] = data.at(1);
			}
		}

	private:

		std::map<UUID, CompT> mComponents; // Map of entity id to component

	};

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData() = default;
		explicit SceneData(fs::path  path) : mPath(std::move(path)) {}

		// Initialize ECS & SceneGraph with loaded data
		void Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph);

		void UpdateData(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph);

		void Clear();

		// Save Entities/Components to json scene file
		void Save();

		// Load  Entities/Components from Binary Scene File
		void Load(const bool forceLoad = false);
		void LoadAndInit(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph);

		void SetPath(const fs::path& path);
		const fs::path& GetPath();

		template<typename CompT>
		void RegisterComponent()
		{
			const char* typeName = typeid(CompT).name();

			assert(mComponentData.find(typeName) == mComponentData.end() && "SceneData::RegisterComponent() - Registering component type more than once");

			// Create
			std::shared_ptr<ComponentDataArray<CompT>> sceneDataArray = std::make_shared<ComponentDataArray<CompT>>();

			mComponentData.insert({ typeName, std::static_pointer_cast<IComponentDataArray>(sceneDataArray) });
		}

	private:

		fs::path mPath;
		bool mHasData = false; // This scene contains a copy of active scene data, either loaded from file or copied from ecs

		std::vector<UUID> mEntityIDs;
		std::unordered_map<std::string, std::shared_ptr<IComponentDataArray>> mComponentData; // Map of component data arrays

		std::vector<UUID> mRootNodeIDs;

		std::vector<UUID> mNodeIDs;
		std::unordered_map<UUID, std::string> mNodeIDToType;
		std::unordered_map<UUID, json> mNodeIDToJson;
		std::unordered_map<UUID, std::vector<UUID>> mChildNodeIDs;

		void SerializeNodeAndChildren(scene::SceneGraphSubsystem* sceneGraph, UUID id);
	};

    class ISceneComponentRegister
    {
    public:

        virtual ~ISceneComponentRegister() = default;

        virtual void RegisterComponentWithScene(std::shared_ptr<SceneData> scene_data) = 0;

    };

    template<typename T>
    class SceneComponentRegister : public ISceneComponentRegister
    {
    public:

        SceneComponentRegister() = default;
        ~SceneComponentRegister() override = default;

        void RegisterComponentWithScene(const std::shared_ptr<SceneData> sceneData) override
        {
            sceneData->RegisterComponent<T>();
        }

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

		template<typename T>
		void RegisterComponent()
		{
			auto type = entt::resolve<T>();
			auto typeName = type.info().name();

			/*if (mComponentRegisters.find(typeName) == mComponentRegisters.end())
			{
				auto sceneCompRegister = std::make_shared<SceneComponentRegister<T>>();

				mComponentRegisters.insert({ typeName, std::static_pointer_cast<ISceneComponentRegister>(sceneCompRegister) });
			}*/
		}

	private:

		std::shared_ptr<SceneData> mCurrentSceneData = nullptr;
        std::unordered_map<fs::path, std::shared_ptr<SceneData>> mSceneData;
        std::unordered_map<std::string, std::shared_ptr<ISceneComponentRegister>> mComponentRegisters;

	};
}

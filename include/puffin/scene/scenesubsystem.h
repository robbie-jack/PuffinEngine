#pragma once

#include <fstream>
#include <filesystem>
#include <utility>

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"

#include "puffin/assets/assetregistry.h"
#include "puffin/scene/scenegraph.h"
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

		virtual void init(ecs::EnTTSubsystem* entt_subsystem) = 0;
		virtual void update(ecs::EnTTSubsystem* entt_subsystem) = 0;
		virtual void clear() = 0;
		virtual size_t size() = 0;
		[[nodiscard]] virtual json save_to_json() const = 0;
		virtual void load_from_json(const json& componentData) = 0;
	};

	template<typename CompT>
	class ComponentDataArray : public IComponentDataArray
	{
	public:

		ComponentDataArray() = default;
		~ComponentDataArray() override = default;

		void init(ecs::EnTTSubsystem* entt_subsystem) override
		{
			const auto registry = entt_subsystem->registry();

			for (const auto& [id, comp] : m_components)
			{
				const auto entity = entt_subsystem->get_entity(id);

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

		void update(ecs::EnTTSubsystem* entt_subsystem) override
		{
			clear();

			const auto registry = entt_subsystem->registry();

			for (const auto& [ entity, comp ] : registry->view<const CompT>().each())
			{
				const auto& id = entt_subsystem->get_id(entity);

				if (entt_subsystem->should_be_serialized(id))
				{
					m_components.emplace(id, comp);
				}
			}
		}

		void clear() override
		{
			m_components.clear();
		}

		size_t size() override
		{
			return m_components.size();
		}

		[[nodiscard]] json save_to_json() const override
		{
			json data;

			int i = 0;
			for (auto& [id, comp] : m_components)
			{
				data[i] = { id, comp };

				i++;
			}

			return data;
		}

		void load_from_json(const json& component_data) override
		{
			for (const auto& data : component_data)
			{
				PuffinID id = data.at(0);

				m_components[id] = data.at(1);
			}
		}

	private:

		std::map<PuffinID, CompT> m_components; // Map of entity id to component

	};

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData() = default;
		explicit SceneData(fs::path  path) : m_path(std::move(path)) {}

		// Initialize ECS & SceneGraph with loaded data
		void setup(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph);

		void update_data(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph);

		void clear();

		// Save Entities/Components to json scene file
		void save();

		// Load  Entities/Components from Binary Scene File
		void load(const bool forceLoad = false);
		void load_and_init(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph);

		void set_path(const fs::path& path);
		const fs::path& path();

		template<typename CompT>
		void register_component()
		{
			const char* typeName = typeid(CompT).name();

			assert(m_component_data.find(typeName) == m_component_data.end() && "Registering component type more than once");

			// Create
			std::shared_ptr<ComponentDataArray<CompT>> sceneDataArray = std::make_shared<ComponentDataArray<CompT>>();

			m_component_data.insert({ typeName, std::static_pointer_cast<IComponentDataArray>(sceneDataArray) });
		}

	private:

		fs::path m_path;
		bool m_has_data = false; // This scene contains a copy of active scene data, either loaded from file or copied from ecs

		std::vector<PuffinID> m_entity_ids;
		std::unordered_map<std::string, std::shared_ptr<IComponentDataArray>> m_component_data; // Map of component data arrays

		std::vector<PuffinID> m_root_node_ids;

		std::vector<PuffinID> m_node_ids;
		std::unordered_map<PuffinID, std::string> m_node_id_to_type;
		std::unordered_map<PuffinID, json> m_node_id_to_json;
		std::unordered_map<PuffinID, std::vector<PuffinID>> m_child_node_ids;

		void add_node_id_and_child_ids(scene::SceneGraphSubsystem* scene_graph, PuffinID id);
	};

    class ISceneComponentRegister
    {
    public:

        virtual ~ISceneComponentRegister() = default;

        virtual void register_component_with_scene(std::shared_ptr<SceneData> scene_data) = 0;

    };

    template<typename T>
    class SceneComponentRegister : public ISceneComponentRegister
    {
    public:

        SceneComponentRegister() = default;
        ~SceneComponentRegister() override = default;

        void register_component_with_scene(std::shared_ptr<SceneData> scene_data) override
        {
            scene_data->register_component<T>();
        }

    };

	class SceneSubsystem : public core::Subsystem
	{
	public:

		explicit SceneSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneSubsystem() override;

		void Initialize(core::SubsystemManager* subsystem_manager) override;
		void Deinitialize() override;

		void BeginPlay() override;
		void EndPlay() override;

		void load() const;
		void setup() const;
		void load_and_setup() const;

		std::shared_ptr<SceneData> create_scene(const fs::path& path);

		std::shared_ptr<SceneData> scene_data();

		template<typename T>
		void register_component()
		{
			const char* type_name = typeid(T).name();

			assert(m_component_registers.find(type_name) == m_component_registers.end() && "Registering component type more than once");

			// Create
			auto scene_comp_register = std::make_shared<SceneComponentRegister<T>>();

			m_component_registers.insert({ type_name, std::static_pointer_cast<ISceneComponentRegister>(scene_comp_register) });
		}

		

	private:

		std::shared_ptr<SceneData> m_current_scene_data = nullptr;
        std::unordered_map<fs::path, std::shared_ptr<SceneData>> m_scenes;
        std::unordered_map<std::string, std::shared_ptr<ISceneComponentRegister>> m_component_registers;

	};
}

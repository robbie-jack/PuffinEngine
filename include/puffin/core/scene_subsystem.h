#pragma once

#include <fstream>
#include <filesystem>
#include <utility>

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"

#include "puffin/assets/asset_registry.h"
#include "puffin/scene/scene_graph.h"
#include "puffin/core/system.h"
#include "puffin/ecs/entt_subsystem.h"
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

		virtual void init(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
		virtual void update(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
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

		void init(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem) override
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

		void update(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem) override
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
		void init(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem, const std::shared_ptr<scene::SceneGraph>& scene_graph)
		{
			// Add entities to registry/subsystem
			for (const auto& id : m_entity_ids)
			{
				entt_subsystem->add_entity(id);
			}

			// Init components
			for (auto& [type, compArray] : m_component_data)
			{
				compArray->init(entt_subsystem);
			}

			// Add nodes to scene graph
			for (const auto& id : m_root_node_ids)
			{
				scene_graph->add_node(m_node_id_to_type.at(id).c_str(), id);

				if (m_child_node_ids.count(id) != 0)
				{
					for (const auto& child_id : m_child_node_ids.at(id))
					{
						scene_graph->add_child_node(m_node_id_to_type.at(child_id).c_str(), child_id, id);
					}
				}
			}

			for (const auto& id : m_node_ids)
			{
				auto node = scene_graph->get_node_ptr(id);
				node->deserialize(m_node_id_to_json.at(id));
			}
		}

		void update_data(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem, const std::shared_ptr<scene::SceneGraph>& scene_graph)
		{
			clear();

			const auto registry = entt_subsystem->registry();

			registry->each([&](auto entity)
			{
				const auto& id = entt_subsystem->get_id(entity);

				m_entity_ids.push_back(id);
			});

			for (auto& [type, compArray] : m_component_data)
			{
				compArray->update(entt_subsystem);
			}

			for (auto id : scene_graph->get_root_node_ids())
			{
				m_root_node_ids.push_back(id);

				add_node_id_and_child_ids(scene_graph, id);
			}

			m_has_data = true;
		}

		void clear()
		{
			m_entity_ids.clear();

			for (auto& [type, compArray] : m_component_data)
			{
				compArray->clear();
			}

			m_node_ids.clear();
			m_node_id_to_type.clear();
			m_node_id_to_json.clear();

			m_root_node_ids.clear();
			m_child_node_ids.clear();

			m_has_data = false;
		}

		template<typename CompT>
		void register_component()
		{
			const char* typeName = typeid(CompT).name();

			assert(m_component_data.find(typeName) == m_component_data.end() && "Registering component type more than once");

			// Create
			std::shared_ptr<ComponentDataArray<CompT>> sceneDataArray = std::make_shared<ComponentDataArray<CompT>>();

			m_component_data.insert({ typeName, std::static_pointer_cast<IComponentDataArray>(sceneDataArray) });
		}

		// Save Entities/Components to json scene file
		void save()
		{
			// Write scene data to json file
			
			json data;
			data["entity_ids"] = m_entity_ids;

			for (auto& [type, comp_array] : m_component_data)
			{
				if (comp_array->size() > 0)
					data[type] = comp_array->save_to_json();
			}

			data["root_node_ids"] = m_root_node_ids;
			data["node_ids"] = m_node_ids;
			data["node_id_to_type"] = m_node_id_to_type;
			data["node_id_to_json"] = m_node_id_to_json;
			data["child_node_ids"] = m_child_node_ids;

			if (!fs::exists(m_path.parent_path()))
			{
				fs::create_directories(m_path.parent_path());
			}

			std::ofstream os(m_path, std::ios::out);

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		// Load  Entities/Components from Binary Scene File
		void load(const bool forceLoad = false)
		{
			if (m_has_data && !forceLoad)
				return;

			if (!fs::exists(m_path))
				return;

			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(m_path);

			json data;
			is >> data;

			is.close();

			m_entity_ids = data.at("entity_ids").get<std::vector<PuffinID>>();

			for (auto& [type, comp_array] : m_component_data)
			{
				if (data.contains(type))
				{
					comp_array->load_from_json(data.at(type));
				}
			}

			m_root_node_ids = data.at("root_node_ids").get<std::vector<PuffinID>>();
			m_node_ids = data.at("node_ids").get<std::vector<PuffinID>>();
			m_node_id_to_type = data.at("node_id_to_type").get<std::unordered_map<PuffinID, std::string>>();
			m_node_id_to_json = data.at("node_id_to_json").get<std::unordered_map<PuffinID, json>>();
			m_child_node_ids = data.at("child_node_ids").get<std::unordered_map<PuffinID, std::vector<PuffinID>>>();

			m_has_data = true;
		}

		void load_and_init(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem, const std::shared_ptr<scene::SceneGraph>& scene_graph)
		{
			load();
			init(entt_subsystem, scene_graph);
		}

		void set_path(const fs::path& path)
		{
			m_path = path;
		}

		const fs::path& path()
		{
			return m_path;
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

		void add_node_id_and_child_ids(const std::shared_ptr<scene::SceneGraph>& scene_graph, PuffinID id)
		{
			auto node = scene_graph->get_node_ptr(id);

			json json;
			node->serialize(json);

			std::vector<PuffinID> child_ids;
			node->get_child_ids(child_ids);

			m_node_ids.push_back(id);
			m_node_id_to_type.insert({ id, scene_graph->get_node_type_name(id) });
			m_node_id_to_json.insert({ id, json });

			if (!child_ids.empty())
				m_child_node_ids.insert({ id, child_ids });

			for (auto child_id : child_ids)
			{
				add_node_id_and_child_ids(scene_graph, child_id);
			}
		}

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

	class SceneSubsystem : public core::System
	{
	public:

		explicit SceneSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			m_engine->register_callback(core::ExecutionStage::Startup, [&] { init(); }, "SceneSubsystem: init", 200);
			m_engine->register_callback(core::ExecutionStage::BeginPlay, [&] { begin_play(); }, "SceneSubsystem: begin_play", 0);
			m_engine->register_callback(core::ExecutionStage::EndPlay, [&] { load_and_init(); }, "SceneSubsystem: load_and_init", 200);
		}

		~SceneSubsystem() override { m_engine = nullptr; }

        template<typename T>
        void register_component()
        {
            const char* type_name = typeid(T).name();

            assert(m_component_registers.find(type_name) == m_component_registers.end() && "Registering component type more than once");

            // Create
            auto scene_comp_register = std::make_shared<SceneComponentRegister<T>>();

            m_component_registers.insert({type_name, std::static_pointer_cast<ISceneComponentRegister>(scene_comp_register) });
        }

		void load() const
		{
			m_current_scene_data->load();
		}

		void init() const
		{
			const auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
			const auto scene_graph = m_engine->get_system<scene::SceneGraph>();

			m_current_scene_data->init(entt_subsystem, scene_graph);
		}

		void load_and_init() const
		{
			load();
			init();
		}

		void begin_play() const
		{
			const auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
			const auto scene_graph = m_engine->get_system<scene::SceneGraph>();

			m_current_scene_data->update_data(entt_subsystem, scene_graph);
		}

		std::shared_ptr<SceneData> create_scene(const fs::path& path)
		{
            auto scene_path = assets::AssetRegistry::get()->contentRoot() / path;

            if (m_scenes.find(scene_path) == m_scenes.end())
            {
                m_scenes.emplace(scene_path, std::make_shared<SceneData>(scene_path));

                for (auto& [type_name, scene_comp_register] : m_component_registers)
                {
                    scene_comp_register->register_component_with_scene(m_scenes.at(scene_path));
                }
            }

            m_current_scene_data = m_scenes.at(scene_path);

			return m_current_scene_data;
		}

		std::shared_ptr<SceneData> scene_data() { return m_current_scene_data; }

	private:

		std::shared_ptr<SceneData> m_current_scene_data = nullptr;
        std::unordered_map<fs::path, std::shared_ptr<SceneData>> m_scenes;
        std::unordered_map<std::string, std::shared_ptr<ISceneComponentRegister>> m_component_registers;

	};
}
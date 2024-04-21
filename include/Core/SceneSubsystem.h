#pragma once

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"

#include "Core/System.h"
#include "ECS/EnTTSubsystem.h"
#include "Types/UUID.h"

#include <fstream>
#include <filesystem>

#include "scene/scene_graph.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace puffin::io
{
	using SceneComponentType = uint16_t;

	// Interface for array to store scene data
	class IComponentDataArray
	{
	public:

		virtual ~IComponentDataArray() = default;

		virtual void init(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
		virtual void update(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
		virtual void clear() = 0;
		virtual size_t size() = 0;
		virtual json save_to_json() const = 0;
		virtual void load_from_json(const json& componentData) = 0;
	};

	template<typename CompT>
	class ComponentDataArray : public IComponentDataArray
	{
	public:

		ComponentDataArray() {}
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

			registry->each([&](auto entity)
			{
				const auto& id = entt_subsystem->get_id(entity);

				if (registry->any_of<CompT>(entity))
				{
					m_components.emplace(id, registry->get<CompT>(entity));
				}
			});
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
		SceneData(const fs::path& path) : m_path(path) {};

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

		void updateData(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem, const std::shared_ptr<scene::SceneGraph>& scene_graph)
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



			m_has_data = true;
		}

		void clear()
		{
			m_entity_ids.clear();

			for (auto& [type, compArray] : m_component_data)
			{
				compArray->clear();
			}

			m_has_data = false;
		}

		template<typename CompT>
		void registerComponent()
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
			data["IDs"] = m_entity_ids;

			for (auto& [fst, snd] : m_component_data)
			{
				if (snd->size() > 0)
					data[fst] = snd->save_to_json();
			}

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

			m_entity_ids = data.at("IDs").get<std::vector<PuffinID>>();

			for (auto& [type, compArray] : m_component_data)
			{
				if (data.contains(type))
				{
					compArray->load_from_json(data.at(type));
				}
			}

			m_has_data = true;
		}

		void loadAndInit(const std::shared_ptr<ecs::EnTTSubsystem>& entt_subsystem, const std::shared_ptr<scene::SceneGraph>& scene_graph)
		{
			load();
			init(entt_subsystem, scene_graph);
		}

		void setPath(const fs::path& path)
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

		std::vector<PuffinID> m_node_ids;
		std::unordered_map<PuffinID, std::string> m_node_id_to_type;
		std::unordered_map<PuffinID, json> m_node_id_to_json;

		std::vector<PuffinID> m_root_node_ids;
		std::unordered_map<PuffinID, std::vector<PuffinID>> m_child_node_ids;

	};

	class SceneSubsystem : public core::System
	{
	public:

		SceneSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			//mEngine->registerCallback(core::ExecutionStage::Startup, [&] { loadAndInit(); }, "SceneSubsystem: LoadAndInit", 200);
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "SceneSubsystem: BeginPlay", 0);
			//mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { loadAndInit(); }, "SceneSubsystem: LoadAndInit", 200);
		}

		~SceneSubsystem() override { mEngine = nullptr; }

		void loadAndInit() const
		{
			const auto entt_subsystem = mEngine->getSystem<ecs::EnTTSubsystem>();
			const auto scene_graph = mEngine->getSystem<scene::SceneGraph>();

			m_scene_data->load();
			m_scene_data->init(entt_subsystem, scene_graph);
		}

		void beginPlay() const
		{
			const auto entt_subsystem = mEngine->getSystem<ecs::EnTTSubsystem>();
			const auto scene_graph = mEngine->getSystem<scene::SceneGraph>();

			m_scene_data->updateData(entt_subsystem, scene_graph);
		}

		std::shared_ptr<SceneData> createScene(const fs::path& path)
		{
			m_scene_data = std::make_shared<SceneData>(path);

			return m_scene_data;
		}

		std::shared_ptr<SceneData> sceneData() { return m_scene_data; }

	private:

		std::shared_ptr<SceneData> m_scene_data = nullptr;

	};
}
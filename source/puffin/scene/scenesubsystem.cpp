#include "puffin/scene/scenesubsystem.h"

namespace puffin::io
{
	void SceneData::setup(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph)
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

	void SceneData::update_data(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph)
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

	void SceneData::clear()
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

	void SceneData::save()
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

	void SceneData::load(const bool forceLoad)
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

		m_entity_ids = data.at("entity_ids").get<std::vector<UUID>>();

		for (auto& [type, comp_array] : m_component_data)
		{
			if (data.contains(type))
			{
				comp_array->load_from_json(data.at(type));
			}
		}

		m_root_node_ids = data.at("root_node_ids").get<std::vector<UUID>>();
		m_node_ids = data.at("node_ids").get<std::vector<UUID>>();
		m_node_id_to_type = data.at("node_id_to_type").get<std::unordered_map<UUID, std::string>>();
		m_node_id_to_json = data.at("node_id_to_json").get<std::unordered_map<UUID, json>>();
		m_child_node_ids = data.at("child_node_ids").get<std::unordered_map<UUID, std::vector<UUID>>>();

		m_has_data = true;
	}

	void SceneData::load_and_init(ecs::EnTTSubsystem* entt_subsystem, scene::SceneGraphSubsystem* scene_graph)
	{
		load();
		setup(entt_subsystem, scene_graph);
	}

	void SceneData::set_path(const fs::path& path)
	{
		m_path = path;
	}

	const fs::path& SceneData::path()
	{
		return m_path;
	}

	void SceneData::add_node_id_and_child_ids(scene::SceneGraphSubsystem* scene_graph, UUID id)
	{
		auto node = scene_graph->get_node_ptr(id);

		json json;
		node->serialize(json);

		std::vector<UUID> child_ids;
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

	SceneSubsystem::SceneSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "SceneSubsystem";
	}

	SceneSubsystem::~SceneSubsystem()
	{
		mEngine = nullptr;
	}

	void SceneSubsystem::Initialize(core::SubsystemManager* subsystem_manager)
	{
		const auto entt_subsystem = subsystem_manager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto scene_graph = subsystem_manager->CreateAndInitializeSubsystem<scene::SceneGraphSubsystem>();
	}

	void SceneSubsystem::Deinitialize()
	{
	}

	void SceneSubsystem::BeginPlay()
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto scene_graph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		m_current_scene_data->update_data(entt_subsystem, scene_graph);
	}

	void SceneSubsystem::EndPlay()
	{
		load_and_setup();
	}

	void SceneSubsystem::load() const
	{
		m_current_scene_data->load();
	}

	void SceneSubsystem::setup() const
	{
		const auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto scene_graph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		m_current_scene_data->setup(entt_subsystem, scene_graph);
	}

	void SceneSubsystem::load_and_setup() const
	{
		load();
		setup();
	}

	std::shared_ptr<SceneData> SceneSubsystem::create_scene(const fs::path& path)
	{
		auto scene_path = (assets::AssetRegistry::Get()->GetContentRoot() / path).make_preferred();

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

	std::shared_ptr<SceneData> SceneSubsystem::scene_data()
	{
		return m_current_scene_data;
	}
}

#include "puffin/scene/scene_graph.h"

#include "puffin/core/engine.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/nodes/transform_node_2d.h"
#include "puffin/nodes/transform_node_3d.h"
#include "puffin/nodes/rendering/camera_node_3d.h"
#include "puffin/nodes/rendering/light_node_3d.h"
#include "puffin/nodes/rendering/mesh_node.h"

namespace puffin::scene
{
	SceneGraphSubsystem::SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine)
	{
		m_scene_graph_updated = true;
	}

	void SceneGraphSubsystem::initialize(core::ISubsystemManager* subsystem_manager)
	{
		EngineSubsystem::initialize(subsystem_manager);

		register_default_node_types();
	}

	void SceneGraphSubsystem::end_play()
	{
		for (auto& id : m_node_ids)
		{
			if (const auto node = get_node_ptr(id); node)
				node->end_play();
		}

		m_node_ids.clear();
		m_id_to_type.clear();
		m_root_node_ids.clear();
		m_nodes_to_destroy.clear();

		m_global_transform_2ds.clear();
		m_global_transform_3ds.clear();

		for (auto [type, node_array] : m_node_arrays)
		{
			node_array->clear();
		}
	}

	void SceneGraphSubsystem::update(double delta_time)
	{
		update_scene_graph();

		update_transforms();
	}

	bool SceneGraphSubsystem::should_update()
	{
		return EngineSubsystem::should_update();
	}

	Node* SceneGraphSubsystem::add_node(const char* type_name, PuffinID id)
	{
		return add_node_internal(type_name, id);
	}

	Node* SceneGraphSubsystem::add_child_node(const char* type_name, PuffinID id, PuffinID parent_id)
	{
		return add_node_internal(type_name, id, parent_id);
	}

	bool SceneGraphSubsystem::is_valid_node(PuffinID id)
	{
		return m_id_to_type.find(id) != m_id_to_type.end();
	}

	Node* SceneGraphSubsystem::get_node_ptr(const PuffinID& id)
	{
		if (!is_valid_node(id))
			return nullptr;

		return get_array(m_id_to_type.at(id).c_str())->get_ptr(id);
	}

	const std::string& SceneGraphSubsystem::get_node_type_name(const PuffinID& id) const
	{
		return m_id_to_type.at(id);
	}

	TransformComponent2D* SceneGraphSubsystem::get_global_transform_2d(const PuffinID& id)
	{
		return &m_global_transform_2ds.at(id);
	}

	TransformComponent3D* SceneGraphSubsystem::get_global_transform_3d(const PuffinID& id)
	{
		return &m_global_transform_3ds.at(id);
	}

	void SceneGraphSubsystem::queue_destroy_node(const PuffinID& id)
	{
		m_nodes_to_destroy.insert(id);
	}

	std::vector<PuffinID>& SceneGraphSubsystem::get_node_ids()
	{
		return m_node_ids;
	}

	std::vector<PuffinID>& SceneGraphSubsystem::get_root_node_ids()
	{
		return m_root_node_ids;
	}

	void SceneGraphSubsystem::register_default_node_types()
	{
		register_node_type<Node>();
		register_node_type<TransformNode2D>();
		register_node_type<TransformNode3D>();
		register_node_type<rendering::MeshNode>();
		register_node_type<rendering::LightNode3D>();
		register_node_type<rendering::CameraNode3D>();
	}

	void SceneGraphSubsystem::update_scene_graph()
	{
		if (!m_nodes_to_destroy.empty())
		{
			for (auto id : m_nodes_to_destroy)
			{
				destroy_node(id);
			}

			for (auto it = m_root_node_ids.end(); it != m_root_node_ids.begin(); --it)
			{
				if (m_nodes_to_destroy.count(*it) > 0)
					m_root_node_ids.erase(it);
			}

			m_nodes_to_destroy.clear();
			m_scene_graph_updated = true;
		}

		if (m_scene_graph_updated)
		{
			m_node_ids.clear();

			for (auto& id : m_root_node_ids)
			{
				add_id_and_child_ids(id, m_node_ids);
			}

			m_scene_graph_updated = false;
		}
	}

	void SceneGraphSubsystem::destroy_node(PuffinID id)
	{
		std::vector<PuffinID> child_ids;

		if (const auto node = get_node_ptr(id); node)
		{
			node->end_play();
		}

		get_array(m_id_to_type.at(id).c_str())->remove(id);

		m_id_to_type.erase(id);

		if (m_global_transform_2ds.contains(id))
			m_global_transform_2ds.erase(id);

		if (m_global_transform_3ds.contains(id))
			m_global_transform_3ds.erase(id);
	}

	void SceneGraphSubsystem::add_id_and_child_ids(PuffinID id, std::vector<PuffinID>& node_ids)
	{
		m_node_ids.push_back(id);

		const auto node = get_node_ptr(id);

		std::vector<PuffinID> child_ids;
		node->get_child_ids(child_ids);

		for (const auto child_id : child_ids)
		{
			add_id_and_child_ids(child_id, node_ids);
		}
	}

	void SceneGraphSubsystem::update_transforms()
	{
		auto entt_subsystem = m_engine->get_engine_subsystem<ecs::EnTTSubsystem>();
		auto registry = entt_subsystem->registry();

		// Update global transforms
		for (auto& id : m_node_ids)
		{
			if (const auto node = get_node_ptr(id); node && node->transform_changed())
			{
				// Make sure children also have the global transform updated
				std::vector<PuffinID> child_ids;
				node->get_child_ids(child_ids);
				for (auto& child_id : child_ids)
				{
					if (const auto child_node = get_node_ptr(child_id); node)
					{
						child_node->set_transform_changed(true);
					}
				}

				if (node->has_transform_2d())
				{
                    auto& global_transform = m_global_transform_2ds.at(id);
                    global_transform.position = { 0.f };
					global_transform.rotation = 0.0f;
					global_transform.scale = { 1.0f };

					auto parent_id = node->parent_id();

					std::vector<PuffinID> transform_ids_to_apply;
					transform_ids_to_apply.push_back(id);

					while (parent_id != gInvalidID)
					{
						if (const auto node = get_node_ptr(parent_id); node)
						{
							transform_ids_to_apply.push_back(parent_id);

							parent_id = node->parent_id();
						}
					}

					for (int i = transform_ids_to_apply.size(); i-- > 0;)
					{
						apply_local_to_global_transform_2d(transform_ids_to_apply[i], global_transform);
					}

					registry->patch<TransformComponent2D>(node->entity());
				}

				if (node->has_transform_3d())
				{
                    auto& global_transform = m_global_transform_3ds.at(id);
					global_transform.position = { 0.f };
					global_transform.orientation_quat = angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0));
					global_transform.orientation_euler_angles = { 0.0f, 0.0f, 0.0f };
					global_transform.scale = { 1.f };

					auto parent_id = node->parent_id();

					std::vector<PuffinID> transform_ids_to_apply;
					transform_ids_to_apply.push_back(id);

					while (parent_id != gInvalidID)
					{
						if (const auto node = get_node_ptr(parent_id); node)
						{
							transform_ids_to_apply.push_back(parent_id);

							parent_id = node->parent_id();
						}
					}

					for (int i = transform_ids_to_apply.size(); i-- > 0;)
					{
						apply_local_to_global_transform_3d(transform_ids_to_apply[i], global_transform);
					}

					registry->patch<TransformComponent3D>(node->entity());
				}

				node->set_transform_changed(false);
			}
		}
	}

	void SceneGraphSubsystem::apply_local_to_global_transform_2d(PuffinID id, TransformComponent2D& global_transform)
	{
		if (const auto node = get_node_ptr(id); node && node->has_transform_2d())
		{
            const auto& local_transform = node->transform_2d();

            global_transform.position += local_transform->position;
            global_transform.rotation += local_transform->rotation;

			if (global_transform.rotation > 360.0f)
				global_transform.rotation -= 360.0f;

            global_transform.scale *= local_transform->scale;
		}
	}

	void SceneGraphSubsystem::apply_local_to_global_transform_3d(PuffinID id, TransformComponent3D& global_transform)
	{
		if (const auto node = get_node_ptr(id); node && node->has_transform_3d())
		{
			const auto& local_transform = node->transform_3d();

            global_transform.position += local_transform->position;

            global_transform.orientation_quat = local_transform->orientation_quat * global_transform.orientation_quat;

			global_transform.orientation_euler_angles += local_transform->orientation_euler_angles;

            global_transform.scale *= local_transform->scale;
		}
	}

	SceneGraphGameplaySubsystem::SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine) : GameplaySubsystem(engine)
	{
	}

	void SceneGraphGameplaySubsystem::initialize(core::ISubsystemManager* subsystem_manager)
	{
		subsystem_manager->create_and_initialize_subsystem<SceneGraphSubsystem>();
	}

	void SceneGraphGameplaySubsystem::update(double delta_time)
	{
		auto scene_graph = m_engine->get_engine_subsystem<SceneGraphSubsystem>();

		for (auto& id : scene_graph->get_node_ids())
		{
			if (const auto node = scene_graph->get_node_ptr(id); node && node->should_update())
				node->update(m_engine->delta_time());
		}
	}

	bool SceneGraphGameplaySubsystem::should_update()
	{
		return true;
	}

	void SceneGraphGameplaySubsystem::fixed_update(double fixed_time)
	{
		auto scene_graph = m_engine->get_engine_subsystem<SceneGraphSubsystem>();

		for (auto& id : scene_graph->get_node_ids())
		{
			if (const auto node = scene_graph->get_node_ptr(id); node && node->should_update())
				node->update_fixed(m_engine->time_step_fixed());
		}
	}

	bool SceneGraphGameplaySubsystem::should_fixed_update()
	{
		return true;
	}
}

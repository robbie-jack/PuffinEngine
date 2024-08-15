#include "puffin/nodes/node.h"

#include "puffin/scene/scenegraph.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"

namespace puffin
{
	Node::Node(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : m_engine(engine), m_node_id(id)
	{
		auto entt_subsystem = m_engine->get_subsystem<ecs::EnTTSubsystem>();
		m_registry = entt_subsystem->registry();
		m_entity = entt_subsystem->add_entity(m_node_id);
	}

    TransformComponent2D* Node::global_transform_2d()
    {
        if (has_transform_2d())
        {
			auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
            return scene_graph_subsystem->get_global_transform_2d(m_node_id);
        }

        return nullptr;
    }

	const TransformComponent2D* Node::global_transform_2d() const
	{
		if (has_transform_2d())
		{
			auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
			return scene_graph_subsystem->get_global_transform_2d(m_node_id);
		}

		return nullptr;
	}

	TransformComponent3D* Node::global_transform_3d()
	{
		if (has_transform_3d())
		{
			auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
			return scene_graph_subsystem->get_global_transform_3d(m_node_id);
		}

		return nullptr;
	}

	const TransformComponent3D* Node::global_transform_3d() const
	{
		if (has_transform_3d())
		{
			auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
			return scene_graph_subsystem->get_global_transform_3d(m_node_id);
		}

		return nullptr;
	}

	void Node::serialize(json& json) const
	{
		json["name"] = m_name;
	}

	void Node::deserialize(const json& json)
	{
		m_name = json.at("name");
	}

	void Node::queue_destroy() const
	{
		auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
		scene_graph_subsystem->queue_destroy_node(m_node_id);
	}

	Node* Node::get_parent() const
	{
		if (m_parent_id != gInvalidID)
		{
			auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
			return scene_graph_subsystem->get_node_ptr(m_parent_id);
		}

		return nullptr;
	}

	void Node::reparent(const PuffinID& id)
	{
		if (m_parent_id != gInvalidID)
		{
			auto parent = get_parent();
			if (parent)
				parent->remove_child_id(m_node_id);
		}

		set_parent_id(id);
	}

	void Node::get_children(std::vector<Node*>& children) const
	{
		children.reserve(m_child_ids.size());

		auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();

		for (auto id : m_child_ids)
		{
			children.push_back(scene_graph_subsystem->get_node_ptr(id));
		}
	}

	void Node::get_child_ids(std::vector<PuffinID>& child_ids) const
	{
		child_ids.reserve(m_child_ids.size());

		for (auto id : m_child_ids)
		{
			child_ids.push_back(id);
		}
	}

	Node* Node::get_child(PuffinID id) const
	{
		auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
		return scene_graph_subsystem->get_node_ptr(id);
	}

	void Node::remove_child(PuffinID id)
	{
		auto scene_graph_subsystem = m_engine->get_subsystem<scene::SceneGraphSubsystem>();
		Node* node = scene_graph_subsystem->get_node_ptr(id);
		node->queue_destroy();

		remove_child_id(id);
	}
}

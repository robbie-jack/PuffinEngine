#include "scene/node.h"

#include "scene/scene_graph.h"
#include "Core/Engine.h"
#include "ECS/EnTTSubsystem.h"

namespace puffin::scene
{
	Node::Node(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : m_engine(engine), m_node_id(id)
	{
		m_scene_graph = m_engine->getSystem<SceneGraph>();
		m_entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();
		m_registry = m_entt_subsystem->registry();

		m_entity = m_entt_subsystem->add_entity(m_node_id);
	}

	const TransformComponent2D& Node::global_transform_2d() const
	{
		if (has_transform_2d())
		{
			return m_scene_graph->get_global_transform_2d(m_node_id);
		}

		return TransformComponent2D();
	}

	const TransformComponent3D& Node::global_transform_3d() const
	{
		if (has_transform_3d())
		{
			return m_scene_graph->get_global_transform_3d(m_node_id);
		}

		return TransformComponent3D();
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
		m_scene_graph->queue_destroy_node(m_node_id);
	}

	Node* Node::get_parent() const
	{
		if (m_parent_id != gInvalidID)
			return m_scene_graph->get_node_ptr(m_parent_id);

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

		for (auto id : m_child_ids)
		{
			children.push_back(m_scene_graph->get_node_ptr(id));
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
		return m_scene_graph->get_node_ptr(id);
	}

	void Node::remove_child(PuffinID id)
	{
		Node* node = m_scene_graph->get_node_ptr(id);
		node->queue_destroy();

		remove_child_id(id);
	}
}

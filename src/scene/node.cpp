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

	void Node::queue_destroy() const
	{
		m_scene_graph->queue_destroy_node(m_node_id);
	}

	Node* Node::get_parent() const
	{
		if (m_parent_id != gInvalidID)
			return m_scene_graph->get_node(m_parent_id);

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

		m_parent_id = id;
	}

	void Node::get_children(std::vector<Node*>& children) const
	{
		children.reserve(m_child_ids.size());

		for (auto id : m_child_ids)
		{
			children.push_back(m_scene_graph->get_node(id));
		}
	}

	Node* Node::get_child(PuffinID id) const
	{
		return m_scene_graph->get_node(id);
	}

	void Node::remove_child(PuffinID id)
	{
		Node* node = m_scene_graph->get_node(id);
		node->queue_destroy();

		remove_child_id(id);
	}

	template <typename T>
	T& Node::add_child()
	{
		T& child = m_scene_graph->add_node<T>();
		auto* node_ptr = static_cast<Node*>(*child);

		m_child_ids.push_back(node_ptr->id());

		return child;
	}
}

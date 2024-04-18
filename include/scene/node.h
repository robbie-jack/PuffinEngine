#pragma once

#include <list>
#include <memory>

#include "ECS/EnTTSubsystem.h"
#include "Types/UUID.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin::scene
{
	class SceneGraph;

	class Node
	{
	public:

		Node(const PuffinID& id = gInvalidID) : m_node_id(id)
		{
			if (m_node_id == gInvalidID)
				m_node_id = generateID();

			m_name = "Node";
		}

		virtual ~Node() = 0;

		void setup(const std::shared_ptr<core::Engine>& engine);

		virtual void begin_play() {}
		virtual void update(double delta_time) {}
		virtual void physics_update(double delta_time) {}
		virtual void end_play() {}

		[[nodiscard]] PuffinID id() const { return m_node_id; }
		[[nodiscard]] const std::string& name() const { return m_name; }
		[[nodiscard]] bool should_update() const { return m_should_update; }

		template<typename T>
		T& get_component()
		{
			return m_entt_subsystem->registry()->get<T>(m_entity);
		}

		template<typename T>
		bool has_component() const
		{
			return m_entt_subsystem->registry()->any_of<T>(m_entity);
		}

		template<typename T>
		T& add_component()
		{
			return m_entt_subsystem->registry()->get_or_emplace<T>(m_entity);
		}

		template<typename T>
		void remove_component() const
		{
			m_entt_subsystem->registry()->remove<T>(m_entity);
		}

		void queue_destroy() const;
		Node* get_parent() const;
		void reparent(const PuffinID& id);
		void get_children(std::vector<Node*>& children) const;
		Node* get_child(PuffinID id) const;

		template<typename T>
		T& add_child();

		void remove_child(PuffinID id);

		// Remove a child id, for internal use only, use remove_child instead
		void remove_child_id(PuffinID id)
		{
			m_child_ids.remove(id);
		}

	protected:

		bool m_should_update = false;
		PuffinID m_node_id = gInvalidID;
		entt::entity m_entity;

		PuffinID m_parent_id = gInvalidID;
		std::list<PuffinID> m_child_ids;
		std::string m_name;

		std::shared_ptr<core::Engine> m_engine = nullptr;
		std::shared_ptr<scene::SceneGraph> m_scene_graph = nullptr;
		std::shared_ptr<ecs::EnTTSubsystem> m_entt_subsystem = nullptr;

	};
}

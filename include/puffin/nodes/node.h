#pragma once

#include <list>
#include <memory>
#include <entt/entity/registry.hpp>

#include "puffin/components/transform_component_2d.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/types/uuid.h"

#include "nlohmann/json.hpp"

namespace puffin
{
	namespace scene
	{
		class SceneGraph;
	}

	namespace core
	{
		class Engine;
	}

	namespace ecs
	{
		class EnTTSubsystem;
	}

	struct TransformComponent2D;
	struct TransformComponent3D;

	class Node
	{
	public:

		explicit Node(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);

		virtual ~Node() = default;

		virtual void begin_play() {}
		virtual void update(const double delta_time) {}
		virtual void physics_update(const double delta_time) {}
		virtual void end_play() {}

		virtual bool has_transform_2d() const { return false; }
		virtual bool has_transform_3d() const { return false; }

		const TransformComponent2D* global_transform_2d() const;
		virtual const TransformComponent2D* transform_2d() const { return nullptr; }
		virtual TransformComponent2D* transform_2d() { return nullptr; }

		const TransformComponent3D* global_transform_3d() const;
		virtual const TransformComponent3D* transform_3d() const { return nullptr; }
		virtual TransformComponent3D* transform_3d() { return nullptr; }

		virtual void serialize(json& json) const;
		virtual void deserialize(const json& json);

		[[nodiscard]] PuffinID id() const { return m_node_id; }
		[[nodiscard]] entt::entity entity() const { return m_entity; }

		[[nodiscard]] const std::string& name() const { return m_name; }
		[[nodiscard]] std::string& name() { return m_name; }
		void set_name(const std::string& name) { m_name = name; }

		[[nodiscard]] bool should_update() const { return m_should_update; }
		[[nodiscard]] bool transform_changed() const { return m_transform_changed; }
		void set_transform_changed(const bool transform_changed) { m_transform_changed = transform_changed; }

		template<typename T>
		T& get_component()
		{
			return m_registry->get<T>(m_entity);
		}

		template<typename T>
		const T& get_component() const
		{
			return m_registry->get<T>(m_entity);
		}

		template<typename T>
		bool has_component() const
		{
			return m_registry->any_of<T>(m_entity);
		}

		template<typename T>
		T& add_component()
		{
			return m_registry->get_or_emplace<T>(m_entity);
		}

		template<typename T>
		void remove_component() const
		{
			m_registry->remove<T>(m_entity);
		}

		void queue_destroy() const;
		[[nodiscard]] Node* get_parent() const;
		void reparent(const PuffinID& id);
		void get_children(std::vector<Node*>& children) const;
		void get_child_ids(std::vector<PuffinID>& child_ids) const;
		[[nodiscard]] Node* get_child(PuffinID id) const;

		void remove_child(PuffinID id);

		PuffinID parent_id() const { return m_parent_id; }

		// Set parent id, for internal use only, use reparent instead
		void set_parent_id(PuffinID id)
		{
			m_parent_id = id;
		}

		// Add a child id, for internal use only, use add_child instead
		void add_child_id(PuffinID id)
		{
			m_child_ids.push_back(id);
		}

		// Remove a child id, for internal use only, use remove_child instead
		void remove_child_id(PuffinID id)
		{
			m_child_ids.remove(id);
		}

	protected:

		bool m_should_update = false;
		bool m_transform_changed = true;

		PuffinID m_node_id = gInvalidID;
		entt::entity m_entity;

		PuffinID m_parent_id = gInvalidID;
		std::list<PuffinID> m_child_ids;
		std::string m_name;

		std::shared_ptr<core::Engine> m_engine = nullptr;
		std::shared_ptr<scene::SceneGraph> m_scene_graph = nullptr;
		std::shared_ptr<ecs::EnTTSubsystem> m_entt_subsystem = nullptr;
		std::shared_ptr<entt::registry> m_registry = nullptr;
	};
}

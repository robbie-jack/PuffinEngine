#pragma once

#include <list>

#include "ECS/EnTTSubsystem.h"
#include "Types/UUID.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin::scene
{
	class Node
	{
	public:

		Node() : m_entity_id(generateID()) {}
		explicit Node(const PuffinID& id) : m_entity_id(id)
		{
			if (m_entity_id == gInvalidID)
				m_entity_id = generateID();
		}

		virtual ~Node() = 0;

		void init(core::Engine* engine)
		{
			m_engine = engine;
			m_entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

			auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

			entt_subsystem->addEntity(m_entity_id);
		}

		virtual void create() = 0;
		virtual void update(double delta_time) = 0;
		virtual void physics_update(double delta_time) = 0;
		virtual void destroy() = 0;

		PuffinID id() const { return m_entity_id; }

	protected:

		PuffinID m_entity_id = gInvalidID;
		PuffinID m_first_child_id = gInvalidID;
		PuffinID m_next_sibling_id = gInvalidID;

		core::Engine* m_engine = nullptr;
		std::shared_ptr<ecs::EnTTSubsystem> m_entt_subsystem = nullptr;

		entt::entity entt_entity()
		{
			return m_entt_subsystem->getEntity(m_entity_id);
		}

	};
}
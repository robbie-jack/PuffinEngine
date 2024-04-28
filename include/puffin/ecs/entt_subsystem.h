#pragma once

#include "puffin/core/system.h"

#include "puffin/types/uuid.h"
#include "puffin/core/engine.h"
#include "entt/entity/registry.hpp"

namespace puffin::ecs
{
	class EnTTSubsystem : public core::System
	{
	public:

		EnTTSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&]() { endPlay(); }, "EnTTSubsystem: EndPlay", 200);

			m_registry = std::make_shared<entt::registry>();
		}

		~EnTTSubsystem() override { mEngine = nullptr; }

		void endPlay()
		{
			m_registry->clear();

			m_id_to_entity.clear();
		}

		// Add an entity using an existing id
		entt::entity add_entity(const PuffinID id)
		{
			if (valid(id))
				return m_id_to_entity.at(id);

			const auto entity = m_registry->create();

			m_id_to_entity.emplace(id, entity);
			m_entity_to_id.emplace(entity, id);

			return entity;
		}

		void remove_entity(const PuffinID id)
		{
			m_registry->destroy(m_id_to_entity[id]);

			m_id_to_entity.erase(id);
		}

		bool valid(const PuffinID id)
		{
			return m_id_to_entity.find(id) != m_id_to_entity.end();
		}

		[[nodiscard]] entt::entity get_entity(const PuffinID& id) const
		{
			const entt::entity& entity = m_id_to_entity.at(id);

			return entity;
		}

		[[nodiscard]] PuffinID get_id(const entt::entity& entity) const
		{
			if (m_entity_to_id.count(entity) != 0)
				return m_entity_to_id.at(entity);

			return gInvalidID;
		}

		std::shared_ptr<entt::registry> registry() { return m_registry; }

	private:

		std::shared_ptr<entt::registry> m_registry = nullptr;

		std::unordered_map<PuffinID, entt::entity> m_id_to_entity;
		std::unordered_map<entt::entity, PuffinID> m_entity_to_id;

	};
}

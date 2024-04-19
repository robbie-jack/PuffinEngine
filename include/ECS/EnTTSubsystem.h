#pragma once

#include "Core/System.h"

#include "Types/UUID.h"
#include "Core/Engine.h"
#include "entt/entity/registry.hpp"
#include "Components/SceneObjectComponent.h"

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
			const auto view = m_registry->view<const SceneObjectComponent>();

			m_registry->destroy(view.begin(), view.end());
			m_registry->clear();

			m_id_to_entity.clear();
		}

		// Add an entity using an existing id
		entt::entity add_entity(const PuffinID id)
		{
			const auto entity = m_registry->create();

			m_id_to_entity.emplace(id, entity);

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

		entt::entity get_entity(const PuffinID id)
		{
			const entt::entity& entity = m_id_to_entity[id];

			return entity;
		}

		std::shared_ptr<entt::registry> registry() { return m_registry; }

	private:

		std::shared_ptr<entt::registry> m_registry = nullptr;

		std::unordered_map<PuffinID, entt::entity> m_id_to_entity;

	};
}

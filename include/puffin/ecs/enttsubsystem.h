#pragma once

#include <unordered_map>
#include <unordered_set>

#include "puffin/core/subsystem.h"

#include "puffin/types/uuid.h"
#include "puffin/core/engine.h"
#include "entt/entity/registry.hpp"

namespace puffin::ecs
{
	class EnTTSubsystem : public core::Subsystem
	{
	public:

		explicit EnTTSubsystem(const std::shared_ptr<core::Engine>& engine);
		~EnTTSubsystem() override;

		void Initialize(core::SubsystemManager* subsystem_manager) override;
		void Deinitialize() override;

		void EndPlay() override;

		PuffinID add_entity(bool should_be_serialized = true);

		// Add an entity using an existing id
		entt::entity add_entity(const PuffinID id, bool should_be_serialized = true);

		void remove_entity(const PuffinID id);

		bool valid(const PuffinID id);

		[[nodiscard]] entt::entity get_entity(const PuffinID& id) const;
		[[nodiscard]] PuffinID get_id(const entt::entity& entity) const;

		[[nodiscard]] bool should_be_serialized(const PuffinID& id) const;

		std::shared_ptr<entt::registry> registry();

	private:

		std::shared_ptr<entt::registry> m_registry = nullptr;

		std::unordered_map<PuffinID, entt::entity> m_id_to_entity;
		std::unordered_map<entt::entity, PuffinID> m_entity_to_id;
		std::unordered_set<PuffinID> m_should_be_serialized;

	};
}

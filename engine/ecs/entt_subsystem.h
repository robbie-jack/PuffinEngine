#pragma once

#include <unordered_map>
#include <unordered_set>

#include "core/subsystem.h"
#include "types/uuid.h"
#include "core/engine.h"
#include "entt/entity/registry.hpp"

namespace puffin::ecs
{
	class EnTTSubsystem : public core::Subsystem
	{
	public:

		explicit EnTTSubsystem(const std::shared_ptr<core::Engine>& engine);
		~EnTTSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void EndPlay() override;

		UUID AddEntity(bool shouldBeSerialized = true);

		// Add an entity using an existing id
		entt::entity AddEntity(UUID id, bool shouldBeSerialized = true);

		void RemoveEntity(UUID id);

		bool IsEntityValid(UUID id);

		[[nodiscard]] entt::entity GetEntity(UUID id) const;
		[[nodiscard]] UUID GetID(entt::entity entity) const;

		[[nodiscard]] bool ShouldEntityBeSerialized(const UUID& id) const;

		std::shared_ptr<entt::registry> GetRegistry();

	private:

		std::shared_ptr<entt::registry> mRegistry = nullptr;

		std::unordered_map<UUID, entt::entity> mIDToEntity;
		std::unordered_map<entt::entity, UUID> mEntityToID;
		std::unordered_set<UUID> mShouldBeSerialized;

	};
}

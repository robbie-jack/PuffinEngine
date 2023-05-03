#pragma once

#include "Engine/Subsystem.hpp"

#include "Types/UUID.h"
#include "entt/entity/registry.hpp"
#include "Components/SceneObjectComponent.hpp"

namespace Puffin::ECS
{
	class EnTTSubsystem : public Core::Subsystem
	{
	public:

		EnTTSubsystem() { m_registry = std::make_shared<entt::registry>(); }
		~EnTTSubsystem() override = default;

		void SetupCallbacks() override {}

		// Create a new entity with a default scene object component
		entt::entity CreateEntity(std::string name)
		{
			auto entity = m_registry->create();

			auto& sceneObject = m_registry->emplace<SceneObjectComponent>(entity, name);

			m_idToEntityMap.emplace(sceneObject.uuid, entity);

			return entity;
		}

		bool IsValid(UUID uuid)
		{
			return m_idToEntityMap.find(uuid) != m_idToEntityMap.end();
		}

		entt::entity GetEntity(UUID uuid)
		{
			entt::entity& entity = m_idToEntityMap[uuid];

			return entity;
		}

		std::shared_ptr<entt::registry> Registry() { return m_registry; }

	private:

		std::shared_ptr<entt::registry> m_registry = nullptr;

		std::unordered_map<UUID, entt::entity> m_idToEntityMap;

	};
}
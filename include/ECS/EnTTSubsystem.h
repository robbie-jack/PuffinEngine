#pragma once

#include "Core/Subsystem.h"

#include "Types/UUID.h"
#include "entt/entity/registry.hpp"
#include "Components/SceneObjectComponent.h"

namespace puffin::ecs
{
	class EnTTSubsystem : public core::Subsystem
	{
	public:

		EnTTSubsystem() { mRegistry = std::make_shared<entt::registry>(); }
		~EnTTSubsystem() override = default;

		void setupCallbacks() override {}

		// Create a new entity with a default scene object component
		entt::entity createEntity(const std::string& name)
		{
			auto entity = mRegistry->create();

			auto& sceneObject = mRegistry->emplace<SceneObjectComponent>(entity, generateID(), name);

			mIdToEntityMap.emplace(sceneObject.id, entity);

			return entity;
		}

		bool valid(const PuffinID uuid)
		{
			return mIdToEntityMap.find(uuid) != mIdToEntityMap.end();
		}

		entt::entity getEntity(const PuffinID uuid)
		{
			const entt::entity& entity = mIdToEntityMap[uuid];

			return entity;
		}

		std::shared_ptr<entt::registry> registry() { return mRegistry; }

	private:

		std::shared_ptr<entt::registry> mRegistry = nullptr;

		std::unordered_map<PuffinID, entt::entity> mIdToEntityMap;

	};
}
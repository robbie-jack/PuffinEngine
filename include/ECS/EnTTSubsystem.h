#pragma once

#include "Core/Subsystem.h"

#include "Types/UUID.h"
#include "entt/entity/registry.hpp"
#include "Components/SceneObjectComponent.h"
#include "Core/Engine.h"

namespace puffin::ecs
{
	class EnTTSubsystem : public core::Subsystem
	{
	public:

		EnTTSubsystem() { mRegistry = std::make_shared<entt::registry>(); }
		~EnTTSubsystem() override = default;

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::Stop, [&]() { stop(); }, "EnTTSubsystem: Stop", 255);
		}

		void stop()
		{
			const auto view = mRegistry->view<const SceneObjectComponent>();

			mRegistry->destroy(view.begin(), view.end());
			mRegistry->clear();

			mIdToEntity.clear();
		}

		// Create a new entity with a default scene object component
		entt::entity createEntity(const std::string& name)
		{
			auto entity = mRegistry->create();

			auto& sceneObject = mRegistry->emplace<SceneObjectComponent>(entity, generateID(), name);

			mIdToEntity.emplace(sceneObject.id, entity);

			return entity;
		}

		// Add an entity using an existing id
		entt::entity addEntity(const PuffinID id)
		{
			const auto entity = mRegistry->create();

			auto& sceneObject = mRegistry->emplace<SceneObjectComponent>(entity, id);

			mIdToEntity.emplace(sceneObject.id, entity);

			return entity;
		}

		bool valid(const PuffinID id)
		{
			return mIdToEntity.find(id) != mIdToEntity.end();
		}

		entt::entity getEntity(const PuffinID id)
		{
			const entt::entity& entity = mIdToEntity[id];

			return entity;
		}

		std::shared_ptr<entt::registry> registry() { return mRegistry; }

	private:

		std::shared_ptr<entt::registry> mRegistry = nullptr;

		std::unordered_map<PuffinID, entt::entity> mIdToEntity;

	};
}

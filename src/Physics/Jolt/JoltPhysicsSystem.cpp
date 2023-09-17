#include "Physics/Jolt/JoltPhysicsSystem.h"

#include "Physics/PhysicsConstants.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace puffin::physics
{
	void JoltPhysicsSystem::beginPlay()
	{
		// Register allocation hook
		JPH::RegisterDefaultAllocator();

		// Create factory
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all Jolt physics types
		JPH::RegisterTypes();

		mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		mInternalPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
		mInternalPhysicsSystem->Init(gMaxShapes, mNumBodyMutexes, mMaxBodyPairs, mMaxContactConstraints,
			mBPLayerInterfaceImpl, mObjectVsBroadphaseLayerFilter, mObjectVsObjectLayerFilter);

		updateComponents();
	}

	void JoltPhysicsSystem::fixedUpdate()
	{
		updateComponents();

		mInternalPhysicsSystem->Update(mEngine->deltaTime(), 1, mTempAllocator.get(), mJobSystem.get());
	}

	void JoltPhysicsSystem::endPlay()
	{
		mInternalPhysicsSystem = nullptr;
		mJobSystem = nullptr;
		mTempAllocator = nullptr;

		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void JoltPhysicsSystem::onConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mBoxesToInit.push_back(object.id);
	}

	void JoltPhysicsSystem::onDestroyBox(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::onConstructSphere(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mSpheresToInit.push_back(object.id);
	}

	void JoltPhysicsSystem::onDestroySphere(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::onConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mBodiesToInit.push_back(object.id);
	}

	void JoltPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::updateComponents()
	{
		const auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

		// Update Spheres
		{
			for (const auto& id : mSpheresToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& sphere = registry->get<const SphereComponent3D>(entity);

				initSphere(object.id, transform, sphere);
			}

			mSpheresToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : mBoxesToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& box = registry->get<const BoxComponent3D>(entity);

				initBox(object.id, transform, box);
			}

			mBoxesToInit.clear();
		}

		// Update Rigidbodies
		{
			for (const auto& id : mBodiesToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& rb = registry->get<const RigidbodyComponent3D>(entity);

				if (mShapeRefs.contains(id))
				{
					initRigidbody(object.id, transform, rb);
				}
			}

			mBodiesToInit.clear();
		}
	}

	void JoltPhysicsSystem::initBox(PuffinID id, const TransformComponent3D& transform, const BoxComponent3D& box)
	{
		if (mInternalPhysicsSystem)
		{
			JPH::BodyInterface& bodyInterface = mInternalPhysicsSystem->GetBodyInterface();

			//JPH::BoxShapeSettings boxShapeSettings();
		}
	}

	void JoltPhysicsSystem::initSphere(PuffinID id, const TransformComponent3D& transform,
		const SphereComponent3D& circle)
	{
		if (mInternalPhysicsSystem)
		{

		}
	}

	void JoltPhysicsSystem::initRigidbody(PuffinID id, const TransformComponent3D& transform,
		const RigidbodyComponent3D& rb)
	{

	}
}

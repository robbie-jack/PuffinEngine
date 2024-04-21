#include "Physics/Jolt/JoltPhysicsSystem.h"

#if PFN_JOLT_PHYSICS

#include <iostream>

#include "Physics/PhysicsConstants.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>

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

		mInternalPhysicsSystem->SetGravity(mGravity);

		updateTimeStep();

		updateComponents();
	}

	void JoltPhysicsSystem::fixedUpdate()
	{
		updateComponents();

		mInternalPhysicsSystem->Update(mFixedTimeStep, mCollisionSteps, mTempAllocator.get(), mJobSystem.get());

		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		// Updated entity position/rotation from simulation
		const auto bodyView = registry->view<TransformComponent3D, VelocityComponent3D, const RigidbodyComponent3D>();

		for (auto [entity, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

			// Update Transform from Rigidbody Position
			registry->patch<TransformComponent3D>(entity, [&](auto& transform)
			{
				transform.position.x = mBodies[id]->GetCenterOfMassPosition().GetX();
				transform.position.y = mBodies[id]->GetCenterOfMassPosition().GetY();
				transform.position.z = mBodies[id]->GetCenterOfMassPosition().GetZ();
				//transform.rotation = maths::radToDeg(-mBodies[id]->GetAngle());
			});

			// Update Velocity with Linear/Angular Velocity#
			registry->patch<VelocityComponent3D>(entity, [&](auto& velocity)
			{
				velocity.linear.x = mBodies[id]->GetLinearVelocity().GetX();
				velocity.linear.y = mBodies[id]->GetLinearVelocity().GetY();
				velocity.linear.z = mBodies[id]->GetLinearVelocity().GetZ();
			});
		}
	}

	void JoltPhysicsSystem::endPlay()
	{
		mBodies.clear();
		mShapeRefs.clear();

		mBodiesToAdd.clear();
		mBodiesToInit.clear();
		mBoxesToInit.clear();
		mSpheresToInit.clear();

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
		const auto id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

		mBoxesToInit.push_back(id);
	}

	void JoltPhysicsSystem::onDestroyBox(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::onConstructSphere(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

		mSpheresToInit.push_back(id);
	}

	void JoltPhysicsSystem::onDestroySphere(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::onConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

		mBodiesToInit.push_back(id);
	}

	void JoltPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSystem::updateComponents()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		// Update Spheres
		{
			for (const auto& id : mSpheresToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& sphere = registry->get<const SphereComponent3D>(entity);

				initSphere(id, transform, sphere);
			}

			mSpheresToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : mBoxesToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& box = registry->get<const BoxComponent3D>(entity);

				initBox(id, transform, box);
			}

			mBoxesToInit.clear();
		}

		// Update Rigidbodies
		{
			// Create Bodies
			for (const auto& id : mBodiesToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& rb = registry->get<const RigidbodyComponent3D>(entity);

				if (mShapeRefs.contains(id))
				{
					initRigidbody(id, transform, rb);
				}
			}

			mBodiesToInit.clear();

			// Add bodies to physics system
			if (!mBodiesToAdd.empty())
			{
				std::vector<JPH::BodyID> bodyIDs;
				bodyIDs.reserve(mBodiesToAdd.size());

				for (const auto& id : mBodiesToAdd)
				{
					bodyIDs.emplace_back(mBodies[id]->GetID());
				}

				mBodiesToAdd.clear();

				JPH::BodyInterface& bodyInterface = mInternalPhysicsSystem->GetBodyInterface();

				const JPH::BodyInterface::AddState addState = bodyInterface.AddBodiesPrepare(bodyIDs.data(), bodyIDs.size());
				bodyInterface.AddBodiesFinalize(bodyIDs.data(), bodyIDs.size(), addState, JPH::EActivation::Activate);
			}
		}
	}

	void JoltPhysicsSystem::initBox(PuffinID id, const TransformComponent3D& transform, const BoxComponent3D& box)
	{
		if (mInternalPhysicsSystem)
		{
			JPH::BoxShapeSettings boxShapeSettings(JPH::Vec3(box.halfExtent.x, box.halfExtent.y, box.halfExtent.z));

			JPH::ShapeSettings::ShapeResult result = boxShapeSettings.Create();

			if (result.HasError())
			{
				std::cout << "Failed to create box shape, id: " << id << " error: " << result.GetError() << std::endl;
				return;
			}

			mShapeRefs.insert(id, result.Get());
		}
	}

	void JoltPhysicsSystem::initSphere(PuffinID id, const TransformComponent3D& transform,
		const SphereComponent3D& circle)
	{
		if (mInternalPhysicsSystem)
		{
			// PUFFIN_TODO - Implement Sphere Creation
		}
	}

	void JoltPhysicsSystem::initRigidbody(PuffinID id, const TransformComponent3D& transform,
		const RigidbodyComponent3D& rb)
	{
		if (mInternalPhysicsSystem && mShapeRefs.contains(id))
		{
			JPH::BodyInterface& bodyInterface = mInternalPhysicsSystem->GetBodyInterface();

			const JPH::EMotionType motionType = gJoltBodyType.at(rb.bodyType);

			JPH::ObjectLayer objectLayer = {};

			if (motionType == JPH::EMotionType::Static)
			{
				objectLayer = JoltLayers::NON_MOVING;
			}
			else
			{
				objectLayer = JoltLayers::MOVING;
			}

			JPH::BodyCreationSettings bodySettings(mShapeRefs[id], JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
				JPH::QuatArg::sIdentity(), motionType, objectLayer);

			bodySettings.mRestitution = rb.elasticity;

			JPH::MassProperties massProperties = bodySettings.GetMassProperties();
			massProperties.ScaleToMass(rb.mass);

			bodySettings.mMassPropertiesOverride = massProperties;
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;

			bodySettings.mFriction = 0.0f;

			mBodies.insert(id, bodyInterface.CreateBody(bodySettings));

			mBodiesToAdd.push_back(id);
		}
	}
}

#endif // PFN_JOLT_PHYSICS
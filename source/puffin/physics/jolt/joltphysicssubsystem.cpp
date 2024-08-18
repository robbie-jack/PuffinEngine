#include "puffin/physics/jolt/joltphysicssubsystem.h"

#if PFN_JOLT_PHYSICS

#include <iostream>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>

#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"

namespace puffin::physics
{
	JoltPhysicsSubsystem::JoltPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "JoltPhysicsSubsystem";
	}

	void JoltPhysicsSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<RigidbodyComponent3D>().connect<&JoltPhysicsSubsystem::OnConstructRigidbody>(this);
		registry->on_destroy<RigidbodyComponent3D>().connect<&JoltPhysicsSubsystem::OnDestroyRigidbody>(this);

		registry->on_construct<RigidbodyComponent3D>().connect<&entt::registry::emplace<VelocityComponent3D>>();
		registry->on_destroy<RigidbodyComponent3D>().connect<&entt::registry::remove<VelocityComponent3D>>();

		registry->on_construct<BoxComponent3D>().connect<&JoltPhysicsSubsystem::OnConstructBox>(this);
		//registry->on_update<BoxComponent3D>().connect<&JoltPhysicsSystem::OnConstructBox>(this);
		registry->on_destroy<BoxComponent3D>().connect<&JoltPhysicsSubsystem::OnDestroyBox>(this);

		registry->on_construct<SphereComponent3D>().connect<&JoltPhysicsSubsystem::OnConstructSphere>(this);
		//registry->on_update<SphereComponent3D>().connect<&JoltPhysicsSystem::onConstructSphere>(this);
		registry->on_destroy<SphereComponent3D>().connect<&JoltPhysicsSubsystem::OnDestroySphere>(this);

		mShapeRefs.reserve(gMaxShapes);

		UpdateTimeStep();
	}

	void JoltPhysicsSubsystem::Deinitialize()
	{
	}

	core::SubsystemType JoltPhysicsSubsystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void JoltPhysicsSubsystem::BeginPlay()
	{
		// Register allocation hook
		JPH::RegisterDefaultAllocator();

		// Create factory
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all Jolt physics types
		JPH::RegisterTypes();

		mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		mJoltPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
		mJoltPhysicsSystem->Init(gMaxShapes, mNumBodyMutexes, mMaxBodyPairs, mMaxContactConstraints,
			mBPLayerInterfaceImpl, mObjectVsBroadphaseLayerFilter, mObjectVsObjectLayerFilter);

		mJoltPhysicsSystem->SetGravity(mGravity);

		UpdateComponents();
	}

	void JoltPhysicsSubsystem::EndPlay()
	{
		mBodies.clear();
		mShapeRefs.clear();

		mBodiesToAdd.clear();
		mBodiesToInit.clear();
		mBoxesToInit.clear();
		mSpheresToInit.clear();

		mJoltPhysicsSystem = nullptr;
		mJobSystem = nullptr;
		mTempAllocator = nullptr;

		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void JoltPhysicsSubsystem::FixedUpdate(double fixedTime)
	{
		UpdateComponents();

		mJoltPhysicsSystem->Update(mFixedTimeStep, mCollisionSteps, mTempAllocator.get(), mJobSystem.get());

		const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		// Updated entity position/rotation from simulation
		const auto bodyView = registry->view<TransformComponent3D, VelocityComponent3D, const RigidbodyComponent3D>();

		for (auto [entity, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

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

	bool JoltPhysicsSubsystem::ShouldFixedUpdate()
	{
		return true;
	}

	void JoltPhysicsSubsystem::OnConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mBoxesToInit.push_back(id);
	}

	void JoltPhysicsSubsystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::OnConstructSphere(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mSpheresToInit.push_back(id);
	}

	void JoltPhysicsSubsystem::OnDestroySphere(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::OnConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mBodiesToInit.push_back(id);
	}

	void JoltPhysicsSubsystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::UpdateTimeStep()
	{
		mFixedTimeStep = mEngine->GetTimeStepFixed();
		mCollisionSteps = static_cast<int>(std::ceil(mFixedTimeStep / mIdealTimeStep));
	}

	void JoltPhysicsSubsystem::UpdateComponents()
	{
		const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		// Update Spheres
		{
			for (const auto& id : mSpheresToInit)
			{
				entt::entity entity = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& sphere = registry->get<const SphereComponent3D>(entity);

				InitSphere(id, transform, sphere);
			}

			mSpheresToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : mBoxesToInit)
			{
				entt::entity entity = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& box = registry->get<const BoxComponent3D>(entity);

				InitBox(id, transform, box);
			}

			mBoxesToInit.clear();
		}

		// Update Rigidbodies
		{
			// Create Bodies
			for (const auto& id : mBodiesToInit)
			{
				entt::entity entity = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& rb = registry->get<const RigidbodyComponent3D>(entity);

				if (mShapeRefs.contains(id))
				{
					InitRigidbody(id, transform, rb);
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

				JPH::BodyInterface& bodyInterface = mJoltPhysicsSystem->GetBodyInterface();

				const JPH::BodyInterface::AddState addState = bodyInterface.AddBodiesPrepare(bodyIDs.data(), bodyIDs.size());
				bodyInterface.AddBodiesFinalize(bodyIDs.data(), bodyIDs.size(), addState, JPH::EActivation::Activate);
			}
		}
	}

	void JoltPhysicsSubsystem::InitBox(UUID id, const TransformComponent3D& transform, const BoxComponent3D& box)
	{
		if (mJoltPhysicsSystem)
		{
			const JPH::BoxShapeSettings boxShapeSettings(JPH::Vec3(box.halfExtent.x, box.halfExtent.y, box.halfExtent.z));

			const JPH::ShapeSettings::ShapeResult result = boxShapeSettings.Create();

			if (result.HasError())
			{
				std::cout << "Failed to create box shape, id: " << id << " error: " << result.GetError() << std::endl;
				return;
			}

			mShapeRefs.emplace(id, result.Get());
		}
	}

	void JoltPhysicsSubsystem::InitSphere(UUID id, const TransformComponent3D& transform,
		const SphereComponent3D& circle)
	{
		if (mJoltPhysicsSystem)
		{
			// PUFFIN_TODO - Implement Sphere Creation
		}
	}

	void JoltPhysicsSubsystem::InitRigidbody(UUID id, const TransformComponent3D& transform,
		const RigidbodyComponent3D& rb)
	{
		if (mJoltPhysicsSystem && mShapeRefs.contains(id))
		{
			JPH::BodyInterface& bodyInterface = mJoltPhysicsSystem->GetBodyInterface();

			const JPH::EMotionType motionType = gPuffinToJoltBodyType.at(rb.body_type);

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

			mBodies.emplace(id, bodyInterface.CreateBody(bodySettings));

			mBodiesToAdd.push_back(id);
		}
	}
}

#endif // PFN_JOLT_PHYSICS
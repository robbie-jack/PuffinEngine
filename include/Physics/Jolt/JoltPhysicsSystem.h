#pragma once

#if PFN_JOLT_PHYSICS

#ifdef PFN_DOUBLE_PRECISION
#define JPH_DOUBLE_PRECISION 1
#endif

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED 1;
#define JPH_PROFILE_ENABLED 1;
#define JPH_DEBUG_RENDERER 1;

#include "Physics/Jolt/JoltPhysicsTypes.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/MotionType.h"

#include "Core/Engine.h"
#include "Core/System.h"
#include "ECS/EnTTSubsystem.h"
#include "Types/PackedArray.h"
#include "Physics/PhysicsConstants.h"
#include "Components/TransformComponent3D.h"
#include "Components/Physics/3D/RigidbodyComponent3D.h"
#include "Components/Physics/3D/ShapeComponents3D.h"
#include "Components/Physics/3D/VelocityComponent3D.h"
#include "Physics/Box2D/Box2DPhysicsSystem.h"

namespace puffin::physics
{
	const inline std::unordered_map<BodyType, JPH::EMotionType> gJoltBodyType =
	{
		{ BodyType::Static, JPH::EMotionType::Static },
		{ BodyType::Kinematic, JPH::EMotionType::Kinematic },
		{ BodyType::Dynamic, JPH::EMotionType::Dynamic }
	};

	class JoltPhysicsSystem : public core::System
	{
	public:

		JoltPhysicsSystem()
		{
			mSystemInfo.name = "JoltPhysicsSystem";

			mShapeRefs.reserve(gMaxShapes);
		}

		~JoltPhysicsSystem() override = default;

		void setup() override
		{
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "JoltPhysicsSystem: BeginPlay");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { fixedUpdate(); }, "JoltPhysicsSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { endPlay(); }, "JoltPhysicsSystem: EndPlay");

			auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

			registry->on_construct<RigidbodyComponent3D>().connect<&JoltPhysicsSystem::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent3D>().connect<&JoltPhysicsSystem::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent3D>().connect<&entt::registry::emplace<VelocityComponent3D>>();
			registry->on_destroy<RigidbodyComponent3D>().connect<&entt::registry::remove<VelocityComponent3D>>();

			registry->on_construct<BoxComponent3D>().connect<&JoltPhysicsSystem::onConstructBox>(this);
			//registry->on_update<BoxComponent3D>().connect<&JoltPhysicsSystem::OnConstructBox>(this);
			registry->on_destroy<BoxComponent3D>().connect<&JoltPhysicsSystem::onDestroyBox>(this);

			registry->on_construct<SphereComponent3D>().connect<&JoltPhysicsSystem::onConstructSphere>(this);
			//registry->on_update<SphereComponent3D>().connect<&JoltPhysicsSystem::onConstructSphere>(this);
			registry->on_destroy<SphereComponent3D>().connect<&JoltPhysicsSystem::onDestroySphere>(this);
		}

		void beginPlay();
		void fixedUpdate();
		void endPlay();

		void onConstructBox(entt::registry& registry, entt::entity entity);
		void onDestroyBox(entt::registry& registry, entt::entity entity);

		void onConstructSphere(entt::registry& registry, entt::entity entity);
		void onDestroySphere(entt::registry& registry, entt::entity entity);

		void onConstructRigidbody(entt::registry& registry, entt::entity entity);
		void onDestroyRigidbody(entt::registry& registry, entt::entity entity);

		void updateTimeStep()
		{
			mFixedTimeStep = mEngine->timeStepFixed();
			mCollisionSteps = static_cast<int>(std::ceil(mFixedTimeStep / mIdealTimeStep));
		}

	private:

		JPH::Vec3Arg mGravity = JPH::Vec3Arg(0.0, -9.81, 0.0);
		double mFixedTimeStep = 0.0;
		const double mIdealTimeStep = 1 / 60.0; // Ideal time step of physics simulation
		int mCollisionSteps = 1; // Number of collision steps doen in physics simulation. Defualts to one, will be set higher if time step is greater than 1 / 60

		const JPH::uint mNumBodyMutexes = 0;
		const JPH::uint mMaxBodyPairs = 65536;
		const JPH::uint mMaxContactConstraints = 10240;

		std::unique_ptr<JPH::PhysicsSystem> mInternalPhysicsSystem;
		std::unique_ptr<JPH::TempAllocatorImpl> mTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;

		JoltBPLayerInterfaceImpl mBPLayerInterfaceImpl;
		JoltObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;
		JoltObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadphaseLayerFilter;

		PackedVector<JPH::ShapeRefC> mShapeRefs;
		PackedVector<JPH::Body*> mBodies;

		std::vector<PuffinID> mBoxesToInit;
		std::vector<PuffinID> mSpheresToInit;
		std::vector<PuffinID> mBodiesToInit;

		std::vector<PuffinID> mBodiesToAdd;

		void updateComponents();

		void initBox(PuffinID id, const TransformComponent3D& transform, const BoxComponent3D& box);
		void initSphere(PuffinID id, const TransformComponent3D& transform, const SphereComponent3D& circle);
		void initRigidbody(PuffinID id, const TransformComponent3D& transform, const RigidbodyComponent3D& rb);
	};
}

#endif // PFN_JOLT_PHYSICS
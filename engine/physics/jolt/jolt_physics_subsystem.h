#pragma once

#if PFN_JOLT_PHYSICS

#ifdef PFN_DOUBLE_PRECISION
#define JPH_DOUBLE_PRECISION 1
#endif

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED 1;
#define JPH_PROFILE_ENABLED 1;
#define JPH_DEBUG_RENDERER 1;

#include "physics/jolt/jolt_physics_types.h"

#include "Jolt/Jolt.h"

#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "core/subsystem.h"
#include "core/engine.h"
#include "ecs/enttsubsystem.h"
#include "physics/body_type.h"
#include "physics/physics_constants.h"
#include "types/storage/mapped_vector.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin::physics
{
	struct RigidbodyComponent3D;
	struct SphereComponent3D;
	struct BoxComponent3D;

	const inline std::unordered_map<BodyType, JPH::EMotionType> gPuffinToJoltBodyType =
	{
		{ BodyType::Static, JPH::EMotionType::Static },
		{ BodyType::Kinematic, JPH::EMotionType::Kinematic },
		{ BodyType::Dynamic, JPH::EMotionType::Dynamic }
	};

	class JoltPhysicsSubsystem : public core::Subsystem
	{
	public:

		explicit JoltPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine);
		~JoltPhysicsSubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		void BeginPlay() override;
		void EndPlay() override;

		void FixedUpdate(double fixedTimeStep) override;
		bool ShouldFixedUpdate() override;

		void OnConstructBox(entt::registry& registry, entt::entity entity);
		void OnDestroyBox(entt::registry& registry, entt::entity entity);

		void OnConstructSphere(entt::registry& registry, entt::entity entity);
		void OnDestroySphere(entt::registry& registry, entt::entity entity);

		void OnConstructRigidbody(entt::registry& registry, entt::entity entity);
		void OnDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		void InitSettingsAndSignals();
		void UpdateComponents();

		void InitBox(UUID id, const TransformComponent3D& transform, const BoxComponent3D& box);
		void InitSphere(UUID id, const TransformComponent3D& transform, const SphereComponent3D& circle);
		void InitRigidbody(UUID id, const TransformComponent3D& transform, const RigidbodyComponent3D& rb);

		bool mEnabled = false;
		JPH::Vec3Arg mGravity = JPH::Vec3Arg(0.0, -9.81, 0.0);
		const double mIdealTimeStep = 1 / 60.0; // Ideal time step of physics simulation

		const JPH::uint mNumBodyMutexes = 0;
		const JPH::uint mMaxBodyPairs = 65536;
		const JPH::uint mMaxContactConstraints = 10240;

		std::unique_ptr<JPH::PhysicsSystem> mJoltPhysicsSystem;
		std::unique_ptr<JPH::TempAllocatorImpl> mTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;

		JoltBPLayerInterfaceImpl mBPLayerInterfaceImpl;
		JoltObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;
		JoltObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadphaseLayerFilter;

		MappedVector<UUID, JPH::ShapeRefC> mShapeRefs;
		MappedVector<UUID, JPH::Body*> mBodies;

		std::vector<UUID> mBoxesToInit;
		std::vector<UUID> mSpheresToInit;
		std::vector<UUID> mBodiesToInit;

		std::vector<UUID> mBodiesToAdd;

		entt::connection mOnConstructRigidbodyConnection, mOnDestroyRigidbodyConnection;
		entt::connection mOnAddVelocityConnection, mOnRemoveVelocityConnection;
		entt::connection mOnConstructSphereConnection, mOnDestroySphereConnection;
		entt::connection mOnConstructBoxConnection, mOnDestroyBoxConnection;
	};
}

#endif // PFN_JOLT_PHYSICS
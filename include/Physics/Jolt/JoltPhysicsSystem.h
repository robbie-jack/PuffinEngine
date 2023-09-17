#pragma once

#ifdef PFN_USE_DOUBLE_PRECISION
#define JPH_DOUBLE_PRECISION 1
#endif

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED 1;
#define JPH_PROFILE_ENABLED 1;
#define JPH_DEBUG_RENDERER 1;

#include "Physics/Jolt/JoltPhysicsTypes.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"

#include "Core/Engine.h"
#include "Core/System.h"
#include "Jolt/Physics/PhysicsSystem.h"

// TODO - Implement JoltPhysicsSystem class

namespace puffin::physics
{
	class JoltPhysicsSystem : public core::System
	{
	public:

		JoltPhysicsSystem()
		{
			mSystemInfo.name = "JoltPhysicsSystem";
		}

		~JoltPhysicsSystem() override = default;

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "JoltPhysicsSystem: BeginPlay");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { fixedUpdate(); }, "JoltPhysicsSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { endPlay(); }, "JoltPhysicsSystem: EndPlay");
		}

		void beginPlay();
		void fixedUpdate();
		void endPlay();

	private:

		const JPH::uint mNumBodyMutexes = 0;
		const JPH::uint mMaxBodyPairs = 65536;
		const JPH::uint mMaxContactConstraints = 10240;

		std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;
		std::unique_ptr<JPH::TempAllocatorImpl> mTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;

		JoltBPLayerInterfaceImpl mBPLayerInterfaceImpl;
		JoltObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;
		JoltObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadphaseLayerFilter;
	};
}
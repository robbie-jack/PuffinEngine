#pragma once

#include "Physics/Jolt/JoltPhysicsTypes.h"

#include "Jolt/Jolt.h"

#include "Core/Engine.h"
#include "Core/System.h"
#include <Jolt/Physics/PhysicsSystem.h>

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

		JPH::PhysicsSystem mPhysicsSystem;

		JoltBPLayerInterfaceImpl mBPLayerInterfaceImpl;
		JoltObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;
		JoltObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadphaseLayerFilter;
	};
}
#include "Physics/Jolt/JoltPhysicsSystem.h"

#include "Physics/PhysicsConstants.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>

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

		mPhysicsSystem.Init(gMaxShapes, mNumBodyMutexes, mMaxBodyPairs, mMaxContactConstraints,
			mBPLayerInterfaceImpl, mObjectVsBroadphaseLayerFilter, mObjectVsObjectLayerFilter);
	}

	void JoltPhysicsSystem::fixedUpdate()
	{

	}

	void JoltPhysicsSystem::endPlay()
	{

	}
}

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

		mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		mPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
		mPhysicsSystem->Init(gMaxShapes, mNumBodyMutexes, mMaxBodyPairs, mMaxContactConstraints,
			mBPLayerInterfaceImpl, mObjectVsBroadphaseLayerFilter, mObjectVsObjectLayerFilter);
	}

	void JoltPhysicsSystem::fixedUpdate()
	{
		mPhysicsSystem->Update(mEngine->deltaTime(), 1, mTempAllocator.get(), mJobSystem.get());
	}

	void JoltPhysicsSystem::endPlay()
	{
		mPhysicsSystem = nullptr;
		mJobSystem = nullptr;
		mTempAllocator = nullptr;

		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}

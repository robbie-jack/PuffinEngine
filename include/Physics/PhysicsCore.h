#pragma once

#include "Core/Engine.h"

#include "Components/Physics/2D/RigidbodyComponent2D.h"
#include "Components/Physics/2D/ShapeComponents2D.h"
#include "Components/Physics/2D/VelocityComponent2D.h"

#include "Components/Physics/3D/RigidbodyComponent3D.h"
#include "Components/Physics/3D/ShapeComponents3D.h"
#include "Components/Physics/3D/VelocityComponent3D.h"

#if PFN_BOX2D_PHYSICS
#include "Physics/Box2D/Box2DPhysicsSystem.h"
#endif

#if PFN_JOLT_PHYSICS
#include "Physics/Jolt/JoltPhysicsSystem.h"
#endif

#if PFN_ONAGER2D_PHYSICS
#include "Physics/Onager2D/OnagerPhysicsSystem2D.h"
#endif

namespace puffin::physics
{
	enum class PhysicsSystem
	{
		Box2D = 0,
		Jolt = 1,
		Onager2D = 2
	};

	inline PhysicsSystem gActivePhysicsSystem = PhysicsSystem::Jolt;

	inline void registerPhysicsSystems(std::shared_ptr<core::Engine> engine)
	{
#if PFN_BOX2D_PHYSICS
		if (gActivePhysicsSystem == PhysicsSystem::Box2D)
		{
			engine->registerSystem<Box2DPhysicsSystem>();
		}
#endif

#if	PFN_JOLT_PHYSICS
		if (gActivePhysicsSystem == PhysicsSystem::Jolt)
		{
			engine->registerSystem<JoltPhysicsSystem>();
		}
#endif

#if PFN_ONAGER2D_PHYSICS
		if (gActivePhysicsSystem == PhysicsSystem::Onager2D)
		{
			engine->registerSystem<OnagerPhysicsSystem2D>();
		}
#endif
	}

	inline void registerComponents(std::shared_ptr<io::SceneData> sceneData)
	{
		sceneData->registerComponent<RigidbodyComponent2D>();
		sceneData->registerComponent<BoxComponent2D>();
		sceneData->registerComponent<CircleComponent2D>();
		sceneData->registerComponent<RigidbodyComponent3D>();
		sceneData->registerComponent<BoxComponent3D>();
		sceneData->registerComponent<SphereComponent3D>();
	}
}
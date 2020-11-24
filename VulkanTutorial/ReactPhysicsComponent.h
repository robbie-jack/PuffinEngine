#pragma once

#include "reactphysics3d/reactphysics3d.h"
#include "BaseComponent.h"

namespace Puffin
{
	namespace Physics
	{
		struct ReactPhysicsComponent : public BaseComponent
		{
			rp3d::RigidBody* body = nullptr;
			rp3d::Transform prevTransform;
		};
	}
}
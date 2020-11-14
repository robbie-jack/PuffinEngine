#pragma once

#include "reactphysics3d/reactphysics3d.h"

namespace Puffin
{
	namespace Physics
	{
		struct ReactPhysicsComponent
		{
			rp3d::RigidBody* body;
			rp3d::Transform prevTransform;
		};
	}
}
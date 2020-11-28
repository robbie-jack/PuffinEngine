#pragma once

#include "btBulletDynamicsCommon.h"
#include "BaseComponent.h"

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent : public BaseComponent
		{
			btCollisionShape* shape;
			btRigidBody* body;

			btVector3 size;
			btScalar mass;
		};
	}
}
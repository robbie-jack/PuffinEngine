#pragma once

#ifndef RIGIDBODY_COMPONENT_H
#define RIGIDBODY_COMPONENT_H

#include "btBulletDynamicsCommon.h"

#include <Components/BaseComponent.h>

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent : public BaseComponent
		{
			RigidbodyComponent()
			{
				bFlagCreated = true;
				bFlagDeleted = false;
			}

			btCollisionShape* shape;
			btRigidBody* body;

			btVector3 size;
			btScalar mass;
		};

		template<class Archive>
		void save(Archive& archive, const RigidbodyComponent& comp)
		{
			archive((float)comp.size.getX(), (float)comp.size.getY(), (float)comp.size.getZ(), (float)comp.mass);
		}

		template<class Archive>
		void load(Archive& archive, RigidbodyComponent& comp)
		{
			float sizeX;
			float sizeY;
			float sizeZ;
			float mass;

			archive(sizeX, sizeY, sizeZ, mass);

			comp.size.setValue((btScalar)sizeX, (btScalar)sizeY, (btScalar)sizeZ);
			comp.mass = (btScalar)mass;
		}
	}
}

#endif // RIGIDBODY_COMPONENT_H
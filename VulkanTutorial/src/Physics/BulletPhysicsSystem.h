#pragma once

#ifndef BULLET_PHYSICS_SYSTEM_H
#define BULLET_PHYSICS_SYSTEM_H

#include <ECS/ECS.h>
#include <btBulletDynamicsCommon.h>
#include <Components/Physics/RigidbodyComponent.h>

namespace Puffin
{
	namespace Physics
	{
		class BulletPhysicsSystem : public ECS::System
		{
		public:

			void Start();
			void Update(float dt);
			void Stop();

			void InitComponent(ECS::Entity entity, btVector3 size = btVector3(1.0f, 1.0f, 1.0f), btScalar mass = 0.0f, btVector3 position = btVector3(0.0f, 0.0f, 0.0f));
			void CleanupComponent(ECS::Entity entity);

		private:

			// Bullet Physics Objects
			btDefaultCollisionConfiguration* collisionConfig;
			btCollisionDispatcher* dispatcher;
			btBroadphaseInterface* broadphaseInterface;
			btSequentialImpulseConstraintSolver* solver;
			btDiscreteDynamicsWorld* physicsWorld;

			btAlignedObjectArray<btCollisionShape*> collisionShapes;
		};
	}
}

#endif // BULLET_PHYSICS_SYSTEM_H
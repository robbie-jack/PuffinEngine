#pragma once

#ifndef BULLET_PHYSICS_SYSTEM_H
#define BULLET_PHYSICS_SYSTEM_H

#include <ECS/ECS.h>
#include <btBulletDynamicsCommon.h>
#include <Components/Physics/RigidbodyComponent.h>
#include <Components/TransformComponent.h>

// Type Includes
#include <Types/RingBuffer.h>

namespace Puffin
{
	namespace Physics
	{
		class BulletPhysicsSystem : public ECS::System
		{
		public:

			void Init();
			void Start();
			void Update(float dt);
			void Stop();

			void InitRigidbody(RigidbodyComponent& rigidbody, TransformComponent& worldTransform);
			void CleanupRigidbody(RigidbodyComponent& rigidbody);

			void CleanupComponent(ECS::Entity entity);

		private:

			// Bullet Physics Objects
			btDefaultCollisionConfiguration* collisionConfig;
			btCollisionDispatcher* dispatcher;
			btBroadphaseInterface* broadphaseInterface;
			btSequentialImpulseConstraintSolver* solver;
			btDiscreteDynamicsWorld* physicsWorld;

			btAlignedObjectArray<btCollisionShape*> collisionShapes;

			void ProcessEvents();
		};
	}
}

#endif // BULLET_PHYSICS_SYSTEM_H
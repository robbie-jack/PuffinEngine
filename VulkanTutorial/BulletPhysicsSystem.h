#pragma once

#include "ECS.h"
#include "btBulletDynamicsCommon.h"
#include "BulletPhysicsComponent.h"

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

			~BulletPhysicsSystem();

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


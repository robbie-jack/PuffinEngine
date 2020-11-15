#pragma once

#include "reactphysics3d/reactphysics3d.h"
#include "ReactPhysicsComponent.h"
#include "ECS.h"

namespace Puffin
{
	namespace Physics
	{
		class ReactPhysicsSystem : public ECS::System
		{
		public:

			void Start();
			bool Update(float dt);
			void Stop();

			void InitComponent(ECS::Entity entity, rp3d::BodyType bodyType = rp3d::BodyType::STATIC, rp3d::Vector3 position = rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::Vector3 rotation = rp3d::Vector3(0.0f, 0.0f, 0.0f));

			~ReactPhysicsSystem();

		private:

			const float timeStep = 1.0f / 60.0f;
			float timeSinceLastUpdate;

			rp3d::PhysicsCommon physicsCommon;
			rp3d::PhysicsWorld* physicsWorld;
		};
	}
}
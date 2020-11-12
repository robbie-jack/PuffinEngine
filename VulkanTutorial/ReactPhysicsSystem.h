#pragma once

#include "System.h"

#include "reactphysics3d/reactphysics3d.h"
#include "ReactPhysicsComponent.h"
#include "TransformSystem.h"

namespace Puffin
{
	namespace Physics
	{
		class ReactPhysicsSystem
		{
		public:

			void Init();
			void Start();
			bool Update(float dt);
			void Stop();
			void SendMessage();

			ReactPhysicsComponent* AddComponent();
			ReactPhysicsComponent* GetComponent(uint32_t entityID);
			void InitComponent(int handle, rp3d::Vector3 position = rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::Vector3 rotation = rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::BodyType bodyType = rp3d::BodyType::DYNAMIC);

			inline std::vector<ReactPhysicsComponent>* GetComponents() { return &physicsComponents; };

			~ReactPhysicsSystem();

		private:

			const float timeStep = 1.0f / 60.0f;
			float timeSinceLastUpdate;

			rp3d::PhysicsCommon physicsCommon;
			rp3d::PhysicsWorld* physicsWorld;

			std::vector<ReactPhysicsComponent> physicsComponents;
		};
	}
}
#pragma once

#include <ECS/ECS.h>
#include <Types/Vector.h>

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponent2D.h>

#include <utility>

namespace Puffin
{
	namespace Physics
	{
		typedef std::pair<ECS::Entity, ECS::Entity> CollisionPair;
		
		class PhysicsSystem2D : public ECS::System
		{
		public:

			void Init(float inTimeStep = 1 / 60.0f);
			void Update(float dt);
			void Cleanup();

		private:

			Vector2 gravity = Vector2(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

			float timeStep = 1.0f / 60.0f; // How often the physics world will update, defaults to 60 times a second
			float accumulatedTime = 0.0f; // Time Accumulated Since last Physics Step

			std::vector<CollisionPair> collisionPairs; // Pairs of entities which should be checked for collisions
			std::vector<Collision2D::Contact> collisionContacts; // Pairs of entities which have collided

			void Step(float dt);

			// Dynamics
			void UpdateDynamics();
			void CalculateImpulseByGravity(RigidbodyComponent2D& Body, const float& dt);

			// Collision Broadphase
			void CollisionBroadphase();

			// Collision Detection
			void CollisionDetection();

			// Collision Resolve
			void CollisionResolve();

		};
	}
}
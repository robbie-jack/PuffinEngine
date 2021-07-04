#include "PhysicsSystem2D.h"

#include <Components/TransformComponent.h>

namespace Puffin
{
	namespace Physics
	{
		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void PhysicsSystem2D::Init(float inTimeStep)
		{
			timeStep = inTimeStep;
			accumulatedTime = 0.0f;
		}

		void PhysicsSystem2D::Update(float dt)
		{
			Step(dt);
		}

		void PhysicsSystem2D::Cleanup()
		{
			
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void PhysicsSystem2D::Step(float dt)
		{
			accumulatedTime += dt;

			if (accumulatedTime >= timeStep)
			{
				accumulatedTime -= timeStep;

				// Update Dynamic Objects
				UpdateDynamics();

				// Perform Collision Broadphase to check if two Colliders can collide
				CollisionBroadphase();

				// Check for Collision between Colliders
				CollisionDetection();

				// Resolve Collision between Colliders
				CollisionResolve();
			}
		}

		void PhysicsSystem2D::UpdateDynamics()
		{
			for (ECS::Entity entity : entityMap["Rigidbody"])
			{
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
				RigidbodyComponent2D& rigidbody = world->GetComponent<RigidbodyComponent2D>(entity);

				// If a body has no mass, then it is kinematic and should not experience forces
				if (rigidbody.mass >= 0)
				{
					// Apply Gravity to Rigidbody Force
					rigidbody.force += gravity * rigidbody.mass;

					// Update Position, Velocity and Acceleration Using Verlet Integration
					Vector2 lastAcceleration = rigidbody.acceleration;

					// Update Position
					transform.position += rigidbody.velocity * timeStep + (rigidbody.acceleration * 0.5 * (timeStep * timeStep));

					// Update Acceleration
					rigidbody.acceleration = rigidbody.force / rigidbody.mass;

					// Update Velocity using average of current and last frames acceleration
					rigidbody.velocity += ((lastAcceleration + rigidbody.acceleration) / 2) * timeStep;

					rigidbody.force = Vector2(0.0f, 0.0f);
				}
			}
		}

		void PhysicsSystem2D::CollisionBroadphase()
		{
			collisionPairs.clear();
			
			// Basic N-Squared Brute Force Broadphase
			for (ECS::Entity entityA : entityMap["Collision"])
			{
				for (ECS::Entity entityB : entityMap["Collision"])
				{
					if (entityA == entityB)
						continue;

					collisionPairs.emplace_back(std::make_pair(entityA, entityB));
				}
			}
		}

		void PhysicsSystem2D::CollisionDetection()
		{
			for (const CollisionPair& collisionPair : collisionPairs)
			{
				
			}
		}

		void PhysicsSystem2D::CollisionResolve()
		{

		}

	}
}
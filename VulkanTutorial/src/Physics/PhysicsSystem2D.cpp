#include "PhysicsSystem2D.h"

#include <Components/TransformComponent.h>

#include <Physics/PhysicsHelpers2D.h>

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

				// Perform Collision2D Broadphase to check if two Colliders can collide
				CollisionBroadphase();

				// Check for Collision2D between Colliders
				CollisionDetection();

				// Resolve Collision2D between Colliders
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
				if (rigidbody.invMass == 0.0f)
					continue;

				CalculateImpulseByGravity(rigidbody, timeStep);

				// Update Position
				transform.position += rigidbody.linearVelocity * timeStep;

				// Update Rotation
				transform.rotation.y += rigidbody.angularVelocity * timeStep;
			}
		}

		void PhysicsSystem2D::CalculateImpulseByGravity(RigidbodyComponent2D& Body, const float& dt)
		{
			if (Body.invMass == 0.0f)
				return;

			Float mass = 1.0 / Body.invMass;
			Vector2 impulseGravity = gravity * mass * dt;
			
			ApplyLinearImpulse(Body, impulseGravity);
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
					
					bool collisionPairAlreadyExists = false;

					for (const CollisionPair& collisionPair : collisionPairs)
					{
						collisionPairAlreadyExists |= collisionPair.first == entityB && collisionPair.second == entityA;
					}

					if (collisionPairAlreadyExists)
						continue;

					collisionPairs.emplace_back(std::make_pair(entityA, entityB));
				}
			}
		}

		void PhysicsSystem2D::CollisionDetection()
		{
			collisionContacts.clear();

			for (const CollisionPair& collisionPair : collisionPairs)
			{
				TransformComponent& transformA = world->GetComponent<TransformComponent>(collisionPair.first);
				ShapeComponent2D& shapeA = world->GetComponent<ShapeComponent2D>(collisionPair.first);

				TransformComponent& transformB = world->GetComponent<TransformComponent>(collisionPair.second);
				ShapeComponent2D& shapeB = world->GetComponent<ShapeComponent2D>(collisionPair.second);

				bool collided = false;
				Collision2D::Contact contact(collisionPair.first, collisionPair.second);

				// Circle Collision
				if (shapeA.type == Collision2D::ShapeType::CIRCLE)
				{
					if (shapeB.type == Collision2D::ShapeType::CIRCLE)
					{
						collided |= Collision2D::TestCircleVsCircle(transformA, shapeA.circle, transformB, shapeB.circle, contact);
					}

					if (shapeB.type == Collision2D::ShapeType::BOX)
					{
						collided |= Collision2D::TestCircleVsBox(transformA, shapeA.circle, transformB, shapeB.box, contact);
					}
				}
				
				// Box Collision
				if (shapeA.type == Collision2D::ShapeType::BOX)
				{
					if (shapeB.type == Collision2D::ShapeType::CIRCLE)
					{
						collided |= Collision2D::TestCircleVsBox(transformB, shapeB.circle, transformA, shapeA.box, contact);
					}

					if (shapeB.type == Collision2D::ShapeType::BOX)
					{
						collided |= Collision2D::TestBoxVsBox(transformA, shapeA.box, transformB, shapeB.box, contact);
					}
				}

				if (collided)
				{
					collisionContacts.emplace_back(contact);
				}
			}
		}

		void PhysicsSystem2D::CollisionResolve()
		{
			for (const Collision2D::Contact& contact : collisionContacts)
			{
				TransformComponent& transformA = world->GetComponent<TransformComponent>(contact.a);
				ShapeComponent2D& shapeA = world->GetComponent<ShapeComponent2D>(contact.a);

				TransformComponent& transformB = world->GetComponent<TransformComponent>(contact.b);
				ShapeComponent2D& shapeB = world->GetComponent<ShapeComponent2D>(contact.b);

				if (world->HasComponent<RigidbodyComponent2D>(contact.a) && world->HasComponent<RigidbodyComponent2D>(contact.b))
				{
					RigidbodyComponent2D& bodyA = world->GetComponent<RigidbodyComponent2D>(contact.a);
					RigidbodyComponent2D& bodyB = world->GetComponent<RigidbodyComponent2D>(contact.b);

					const Float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const Float impulseJ = -(1.0f + elasticity) * vab.Dot(contact.normal) / (bodyA.invMass + bodyB.invMass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					ApplyImpulse(bodyA, shapeA.circle, contact.pointOnA, vectorImpulseJ * 1.0f);
					ApplyImpulse(bodyB, shapeB.circle, contact.pointOnB, vectorImpulseJ * -1.0f);

					// Move colliding bodies to just outside each other
					const Float tA = bodyA.invMass / (bodyA.invMass + bodyB.invMass);
					const Float tB = bodyB.invMass / (bodyA.invMass + bodyB.invMass);

					const Vector2 ds = contact.pointOnB - contact.pointOnA;
					transformA.position.x += ds.x * tA;
					transformA.position.y += ds.y * tA;
					transformB.position.x -= ds.x * tB;
					transformB.position.y -= ds.y * tB;
				}
			}
		}
	}
}
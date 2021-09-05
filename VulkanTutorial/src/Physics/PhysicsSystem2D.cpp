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
			UpdateComponents();
			Step(dt);
		}

		void PhysicsSystem2D::Cleanup()
		{
			
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void PhysicsSystem2D::UpdateComponents()
		{
			for (ECS::Entity entity : entityMap["CircleCollision"])
			{
				if (!world->ComponentInitialized<CircleComponent2D>(entity))
				{
					if (world->HasComponent<RigidbodyComponent2D>(entity))
					{
						world->GetComponent<RigidbodyComponent2D>(entity).shapeType = Collision2D::ShapeType::CIRCLE;
					}

					world->SetComponentInitialized<CircleComponent2D>(entity, true);
				}

				if (world->ComponentDeleted<CircleComponent2D>(entity))
				{
					if (world->HasComponent<RigidbodyComponent2D>(entity))
					{
						world->GetComponent<RigidbodyComponent2D>(entity).shapeType = Collision2D::ShapeType::NONE;
					}

					world->RemoveComponent<CircleComponent2D>(entity);
				}
			}

			for (ECS::Entity entity : entityMap["BoxCollision"])
			{
				if (!world->ComponentInitialized<BoxComponent2D>(entity))
				{
					if (world->HasComponent<RigidbodyComponent2D>(entity))
					{
						world->GetComponent<RigidbodyComponent2D>(entity).shapeType = Collision2D::ShapeType::BOX;
					}

					world->SetComponentInitialized<BoxComponent2D>(entity, true);
				}

				if (world->ComponentDeleted<BoxComponent2D>(entity))
				{
					if (world->HasComponent<RigidbodyComponent2D>(entity))
					{
						world->GetComponent<RigidbodyComponent2D>(entity).shapeType = Collision2D::ShapeType::NONE;
					}

					world->RemoveComponent<BoxComponent2D>(entity);
				}
			}
		}

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
			GenerateCollisionPairs(entityMap["CircleCollision"], entityMap["CircleCollision"]);
		}

		void PhysicsSystem2D::GenerateCollisionPairs(std::set<ECS::Entity>& setA, std::set<ECS::Entity>& setB)
		{
			for (ECS::Entity entityA : setA)
			{
				for (ECS::Entity entityB : setB)
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
				TransformComponent& transformB = world->GetComponent<TransformComponent>(collisionPair.second);

				bool collided = false;
				Collision2D::Contact contact(collisionPair.first, collisionPair.second);

				// Circle Collision
				if (world->HasComponent<CircleComponent2D>(collisionPair.first))
				{
					CircleComponent2D& circleA =  world->GetComponent<CircleComponent2D>(collisionPair.first);

					if (world->HasComponent<CircleComponent2D>(collisionPair.second))
					{
						CircleComponent2D& circleB = world->GetComponent<CircleComponent2D>(collisionPair.first);

						collided |= Collision2D::TestCircleVsCircle(transformA, circleA, transformB, circleB, contact);
					}

					if (world->HasComponent<BoxComponent2D>(collisionPair.second))
					{
						BoxComponent2D& boxB = world->GetComponent<BoxComponent2D>(collisionPair.first);

						collided |= Collision2D::TestCircleVsBox(transformA, circleA, transformB, boxB, contact);
					}
				}
				
				// Box Collision
				if (world->HasComponent<BoxComponent2D>(collisionPair.first))
				{
					BoxComponent2D& boxA = world->GetComponent<BoxComponent2D>(collisionPair.first);

					if (world->HasComponent<CircleComponent2D>(collisionPair.second))
					{
						CircleComponent2D& circleB = world->GetComponent<CircleComponent2D>(collisionPair.first);
						contact.a = collisionPair.second;
						contact.b = collisionPair.first;

						collided |= Collision2D::TestCircleVsBox(transformB, circleB, transformA, boxA, contact);
					}

					if (world->HasComponent<BoxComponent2D>(collisionPair.second))
					{
						BoxComponent2D& boxB = world->GetComponent<BoxComponent2D>(collisionPair.first);

						collided |= Collision2D::TestBoxVsBox(transformA, boxA, transformB, boxB, contact);
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
				TransformComponent& transformB = world->GetComponent<TransformComponent>(contact.b);

				if (world->HasComponent<RigidbodyComponent2D>(contact.a) && world->HasComponent<RigidbodyComponent2D>(contact.b))
				{
					RigidbodyComponent2D& bodyA = world->GetComponent<RigidbodyComponent2D>(contact.a);
					RigidbodyComponent2D& bodyB = world->GetComponent<RigidbodyComponent2D>(contact.b);

					const Float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const Float impulseJ = -(1.0f + elasticity) * vab.Dot(contact.normal) / (bodyA.invMass + bodyB.invMass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					// Apply Impulse to body A
					if (world->HasComponent<CircleComponent2D>(contact.a))
					{
						CircleComponent2D& circle = world->GetComponent<CircleComponent2D>(contact.a);

						ApplyImpulse(bodyA, circle, contact.pointOnA, vectorImpulseJ * 1.0f);
					}

					// Apply Impulse to body B
					if (world->HasComponent<CircleComponent2D>(contact.b))
					{
						CircleComponent2D& circle = world->GetComponent<CircleComponent2D>(contact.b);

						ApplyImpulse(bodyB, circle, contact.pointOnA, vectorImpulseJ * -1.0f);
					}

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
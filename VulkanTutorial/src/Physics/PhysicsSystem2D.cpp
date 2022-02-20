#include "PhysicsSystem2D.h"

#include <Components/TransformComponent.h>
#include "Types/ComponentFlags.h"

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
			for (ECS::Entity entity : entityMap["BoxCollision"])
			{
				auto& box = world->GetComponent<BoxComponent2D>(entity);
				auto& transform = world->GetComponent<TransformComponent>(entity);

				if (world->GetComponentFlag<BoxComponent2D, FlagDirty>(entity))
				{
					// Create default box shape if component does not have one assigned
					if (box.shape_ == nullptr)
					{
						boxShapes_.emplace_back(BoxShape2D());

						box.shape_ = &boxShapes_.back();
					}

					// Create Collider for this component if one does not exist
					if (box.shape_ != nullptr)
						colliders_.emplace_back(new Collision2D::BoxCollider2D(entity, box.shape_));

					world->SetComponentFlag<BoxComponent2D, FlagDirty>(entity, false);
				}
			}

			for (ECS::Entity entity : entityMap["CircleCollision"])
			{
				auto& circle = world->GetComponent<CircleComponent2D>(entity);
				auto& transform = world->GetComponent<TransformComponent>(entity);

				if (world->GetComponentFlag<CircleComponent2D, FlagDirty>(entity))
				{
					// Create default box shape if component does not have one assigned
					if (circle.shape_ == nullptr)
					{
						circleShapes_.emplace_back(CircleShape2D());

						circle.shape_ = &circleShapes_.back();
					}

					if (circle.shape_ != nullptr)
						colliders_.emplace_back(new Collision2D::CircleCollider2D(entity, circle.shape_));

					world->SetComponentFlag<CircleComponent2D, FlagDirty>(entity, false);
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

				// Copy component transform into collider
				for (auto& collider : colliders_)
				{
					const auto& transform = world->GetComponent<TransformComponent>(collider->entity_);

					collider->transform_ = transform;
				}

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

			Float mass = 1.0f / Body.invMass;
			Vector2 impulseGravity = gravity * mass * dt;
			
			ApplyLinearImpulse(Body, impulseGravity);
		}

		void PhysicsSystem2D::CollisionBroadphase()
		{
			collisionPairs.clear();
			
			// Basic N-Squared Brute Force Broadphase
			GenerateCollisionPairs();
		}

		void PhysicsSystem2D::GenerateCollisionPairs()
		{
			for (const Collision2D::Collider2D* colliderA : colliders_)
			{
				for (const Collision2D::Collider2D* colliderB : colliders_)
				{
					if (colliderA->entity_ == colliderB->entity_)
						continue;

					bool collisionPairAlreadyExists = false;

					for (const CollisionPair& collisionPair : collisionPairs)
					{
						collisionPairAlreadyExists |= collisionPair.first->entity_ == colliderB->entity_ && collisionPair.second->entity_ == colliderA->entity_;
					}

					if (collisionPairAlreadyExists)
						continue;

					collisionPairs.emplace_back(std::make_pair(colliderA, colliderB));
				}
			}
		}

		void PhysicsSystem2D::CollisionDetection()
		{
			collisionContacts.clear();

			for (const CollisionPair& collisionPair : collisionPairs)
			{
				Collision2D::Contact contact;

				// Put collision detection here
				if (collisionPair.first->TestCollision(collisionPair.second, contact))
				{
					collisionContacts.emplace_back(contact);
				}
			}
		}

		void PhysicsSystem2D::CollisionResolve()
		{
			for (const Collision2D::Contact& contact : collisionContacts)
			{
				auto& transformA = world->GetComponent<TransformComponent>(contact.a);
				auto& transformB = world->GetComponent<TransformComponent>(contact.b);

				if (world->HasComponent<RigidbodyComponent2D>(contact.a) && world->HasComponent<RigidbodyComponent2D>(contact.b))
				{
					auto& bodyA = world->GetComponent<RigidbodyComponent2D>(contact.a);
					auto& bodyB = world->GetComponent<RigidbodyComponent2D>(contact.b);

					const Float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const Float impulseJ = -(1.0f + elasticity) * vab.Dot(contact.normal) / (bodyA.invMass + bodyB.invMass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					// Apply Impulse to body A
					ApplyImpulse(bodyA, contact.pointOnA, vectorImpulseJ * 1.0f);

					// Apply Impulse to body B
					ApplyImpulse(bodyB, contact.pointOnB, vectorImpulseJ * -1.0f);

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
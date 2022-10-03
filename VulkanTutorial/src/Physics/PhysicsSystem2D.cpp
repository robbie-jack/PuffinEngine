#include "PhysicsSystem2D.h"

#include <Components/TransformComponent.h>
#include "Types/ComponentFlags.h"

#include <Physics/PhysicsHelpers2D.h>

#include "Engine/Engine.hpp"

#include "ECS/Entity.h"

namespace Puffin
{
	namespace Physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		PhysicsSystem2D::PhysicsSystem2D()
		{
			m_boxShapes.Reserve(100);
			m_circleShapes.Reserve(100);
			m_colliders.Reserve(100);
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void PhysicsSystem2D::PreStart()
		{
			std::vector<std::shared_ptr<ECS::Entity>> boxEntites;
			ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntites);
			for (const auto& entity : boxEntites)
			{
				auto& box = entity->GetComponent<BoxComponent2D>();

				if (entity->GetComponentFlag<BoxComponent2D, FlagDirty>())
				{
					InitBox2D(entity->ID(), box);

					entity->SetComponentFlag<BoxComponent2D, FlagDirty>(false);
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
			ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
			for (const auto& entity : circleEntities)
			{
				auto& circle = entity->GetComponent<CircleComponent2D>();

				if (entity->GetComponentFlag<CircleComponent2D, FlagDirty>())
				{
					InitCircle2D(entity->ID(), circle);

					entity->SetComponentFlag<CircleComponent2D, FlagDirty>(false);
				}
			}
		}

		void PhysicsSystem2D::Update()
		{
			UpdateComponents();
			Step();
		}

		void PhysicsSystem2D::Stop()
		{
			m_boxShapes.Clear();
			m_circleShapes.Clear();
			m_colliders.Clear();
			m_collisionPairs.clear();
			m_collisionContacts.clear();

			std::vector<std::shared_ptr<ECS::Entity>> boxEntites;
			ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntites);
			for (const auto& entity : boxEntites)
			{
				auto& box = entity->GetComponent<BoxComponent2D>();

				CleanupBox2D(entity->ID(), box);
			}

			std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
			ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
			for (const auto& entity : circleEntities)
			{
				auto& circle = entity->GetComponent<CircleComponent2D>();

				CleanupCircle2D(entity->ID(), circle);
			}
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void PhysicsSystem2D::InitCircle2D(ECS::EntityID entity, CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
			if (!m_circleShapes.Contains(entity))
			{
				// If this component already has a shape (it's been loaded from the scene file),
				// copy shape vector and switch pointer to new location
				if (circle.shape != nullptr)
				{
					m_circleShapes.Insert(entity, *circle.shape);

					circle.shape = nullptr;
				}
				else
				{
					m_circleShapes.Emplace(entity, CircleShape2D());
				}

				circle.shape = std::shared_ptr<CircleShape2D>(&m_circleShapes[entity]);
			}
			
			// If there is no collider for this entity, create new one
			if (!m_colliders.Contains(entity))
			{
				auto collider = std::make_shared<Collision2D::CircleCollider2D>(entity, circle.shape);

				m_colliders.Insert(entity, collider);
			}
		}

		void PhysicsSystem2D::InitBox2D(ECS::EntityID entity, BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
			if (!m_circleShapes.Contains(entity))
			{
				// If this component already has a shape (it's been loaded from the scene file),
				// copy shape vector and switch pointer to new location
				if (box.shape != nullptr)
				{
					m_boxShapes.Insert(entity, *box.shape);

					box.shape = nullptr;
				}
				else
				{
					m_boxShapes.Emplace(entity, BoxShape2D());
				}

				box.shape = std::shared_ptr<BoxShape2D>(&m_boxShapes[entity]);

				box.shape->UpdatePoints();
			}

			// If there is no collider for this entity, create new one
			if (!m_colliders.Contains(entity))
			{
				auto collider = std::make_shared<Collision2D::BoxCollider2D>(entity, box.shape);

				m_colliders.Insert(entity, collider);
			}
		}

		void PhysicsSystem2D::CleanupCircle2D(ECS::EntityID entity, CircleComponent2D& circle)
		{
			if (m_circleShapes.Contains(entity))
			{
				m_circleShapes.Erase(entity);
				circle.shape = nullptr;
			}

			if (m_colliders.Contains(entity))
			{
				m_colliders.Erase(entity);
			}
		}

		void PhysicsSystem2D::CleanupBox2D(ECS::EntityID entity, BoxComponent2D& box)
		{
			if (m_boxShapes.Contains(entity))
			{
				m_boxShapes.Erase(entity);
				box.shape = nullptr;
			}

			if (m_colliders.Contains(entity))
			{
				m_colliders.Erase(entity);
			}
		}

		void PhysicsSystem2D::UpdateComponents()
		{
			std::vector<std::shared_ptr<ECS::Entity>> rigidbodyEntities;
			ECS::GetEntities<TransformComponent, RigidbodyComponent2D>(m_world, rigidbodyEntities);
			for (const auto& entity : rigidbodyEntities)
			{
				if (entity->GetComponentFlag<RigidbodyComponent2D, FlagDeleted>())
				{
					if (entity->GetComponentFlag<RigidbodyComponent2D, FlagDeleted>())
					{
						entity->RemoveComponent<RigidbodyComponent2D>();
					}
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> boxEntites;
			ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntites);
			for (const auto& entity : boxEntites)
			{
				auto& box = entity->GetComponent<BoxComponent2D>();
				auto& transform = entity->GetComponent<TransformComponent>();

				// Initialize Boxes
				if (entity->GetComponentFlag<BoxComponent2D, FlagDirty>())
				{
					InitBox2D(entity->ID(), box);

					entity->SetComponentFlag<BoxComponent2D, FlagDirty>(false);
				}

				// Cleanup Boxes
				if (entity->GetComponentFlag<BoxComponent2D, FlagDeleted>())
				{
					CleanupBox2D(entity->ID(), box);
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
			ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
			for (const auto& entity : circleEntities)
			{
				auto& circle = entity->GetComponent<CircleComponent2D>();
				auto& transform = entity->GetComponent<TransformComponent>();

				// Initialize Circle
				if (entity->GetComponentFlag<CircleComponent2D, FlagDirty>())
				{
					InitCircle2D(entity->ID(), circle);

					entity->SetComponentFlag<CircleComponent2D, FlagDirty>(false);
				}

				// Cleanup Circles
				if (entity->GetComponentFlag<CircleComponent2D, FlagDeleted>())
				{
					CleanupCircle2D(entity->ID(), circle);
				}
			}
		}

		void PhysicsSystem2D::Step()
		{
			// Update Dynamic Objects
			UpdateDynamics();

			// Copy component transform into collider
			for (auto collider : m_colliders)
			{
				const auto& transform = m_world->GetComponent<TransformComponent>(collider->entity);

				collider->transform = transform;
			}

			// Perform Collision2D Broadphase to check if two Colliders can collide
			CollisionBroadphase();

			// Check for Collision2D between Colliders
			CollisionDetection();

			// Resolve Collision2D between Colliders
			CollisionResponse();
		}

		void PhysicsSystem2D::UpdateDynamics()
		{
			std::vector<std::shared_ptr<ECS::Entity>> rigidbodyEntities;
			ECS::GetEntities<TransformComponent, RigidbodyComponent2D>(m_world, rigidbodyEntities);
			for (const auto& entity : rigidbodyEntities)
			{
				TransformComponent& transform = entity->GetComponent<TransformComponent>();
				RigidbodyComponent2D& rigidbody = entity->GetComponent<RigidbodyComponent2D>();

				// If a body has no mass, then it is kinematic and should not experience forces
				if (rigidbody.invMass == 0.0f)
					continue;

				CalculateImpulseByGravity(rigidbody);

				// Update Position
				transform.position += rigidbody.linearVelocity * m_engine->GetTimeStep();

				// Update Rotation
				transform.rotation.z += rigidbody.angularVelocity * m_engine->GetTimeStep();

				if (transform.rotation.z > 360.0)
				{
					transform.rotation.z = 0;
				}
			}
		}

		void PhysicsSystem2D::CalculateImpulseByGravity(RigidbodyComponent2D& body)
		{
			if (body.invMass == 0.0f)
				return;

			float mass = 1.0f / body.invMass;
			Vector2 impulseGravity = m_gravity * mass * m_engine->GetTimeStep();
			
			ApplyLinearImpulse(body, impulseGravity);
		}

		void PhysicsSystem2D::CollisionBroadphase()
		{
			m_collisionPairs.clear();
			
			// Basic N-Squared Brute Force Broadphase
			GenerateCollisionPairs();
		}

		void PhysicsSystem2D::GenerateCollisionPairs()
		{
			for (const auto colliderA : m_colliders)
			{
				for (const auto colliderB : m_colliders)
				{
					if (colliderA->entity == colliderB->entity)
						continue;

					bool collisionPairAlreadyExists = false;

					for (const CollisionPair& collisionPair : m_collisionPairs)
					{
						collisionPairAlreadyExists |= collisionPair.first->entity == colliderB->entity && collisionPair.second->entity == colliderA->entity;
					}

					if (collisionPairAlreadyExists)
						continue;

					m_collisionPairs.emplace_back(std::make_pair(colliderA, colliderB));
				}
			}
		}

		void PhysicsSystem2D::CollisionDetection()
		{
			m_collisionContacts.clear();

			for (const CollisionPair& collisionPair : m_collisionPairs)
			{
				Collision2D::Contact contact;

				// Put collision detection here
				if (collisionPair.first->TestCollision(collisionPair.second.get(), contact))
				{
					m_collisionContacts.emplace_back(contact);
				}
			}
		}

		void PhysicsSystem2D::CollisionResponse()
		{
			for (const Collision2D::Contact& contact : m_collisionContacts)
			{
				auto& transformA = m_world->GetComponent<TransformComponent>(contact.a);
				auto& transformB = m_world->GetComponent<TransformComponent>(contact.b);

				if (m_world->HasComponent<RigidbodyComponent2D>(contact.a) && m_world->HasComponent<RigidbodyComponent2D>(contact.b))
				{
					auto& bodyA = m_world->GetComponent<RigidbodyComponent2D>(contact.a);
					auto& bodyB = m_world->GetComponent<RigidbodyComponent2D>(contact.b);

					const float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const float nVab = vab.Dot(contact.normal);

					/*if (nVab <= 0)
						continue;*/

					const float impulseJ = -(1.0f + elasticity) * nVab / (bodyA.invMass + bodyB.invMass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					// Apply Impulse to body A
					ApplyImpulse(bodyA, contact.pointOnA, vectorImpulseJ * 1.0f);

					// Apply Impulse to body B
					ApplyImpulse(bodyB, contact.pointOnB, vectorImpulseJ * -1.0f);

					// Calculate impulse caused by friction


					// Move colliding bodies to just outside each other
					const float tA = bodyA.invMass / (bodyA.invMass + bodyB.invMass);
					const float tB = bodyB.invMass / (bodyA.invMass + bodyB.invMass);

					const Vector2 ds = (contact.pointOnB - contact.pointOnA) * -contact.normal;
					transformA.position.x += ds.x * tA;
					transformA.position.y += ds.y * tA;
					transformB.position.x -= ds.x * tB;
					transformB.position.y -= ds.y * tB;
				}
			}
		}
	}
}

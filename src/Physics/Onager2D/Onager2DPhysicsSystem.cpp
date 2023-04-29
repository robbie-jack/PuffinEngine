
#include "Physics/Onager2D/Onager2DPhysicsSystem.h"

#include "Physics/CollisionEvent.h"
#include "Components/TransformComponent.h"
#include "Components/Physics/VelocityComponent.hpp"
#include "Types/ComponentFlags.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Engine/Engine.hpp"
#include "ECS/Entity.h"
#include "Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.hpp"
#include "Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.hpp"

namespace Puffin
{
	namespace Physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		Onager2DPhysicsSystem::Onager2DPhysicsSystem()
		{
			m_boxShapes.Reserve(200);
			m_circleShapes.Reserve(200);
			m_colliders.Reserve(400);

			m_systemInfo.name = "Onager2DPhysicsSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::FixedUpdate;
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::Init()
		{
			auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

			// Register Events
			eventSubsystem->RegisterEvent<CollisionBeginEvent>();
			eventSubsystem->RegisterEvent<CollisionEndEvent>();

			RegisterBroadphase<NSquaredBroadphase>();
			RegisterBroadphase<SweepAndPruneBroadphase>();
			RegisterBroadphase<SpatialHashBroadphase2D>();

			SetBroadphase<SpatialHashBroadphase2D>();
		}

		void Onager2DPhysicsSystem::PreStart()
		{
			std::vector<std::shared_ptr<ECS::Entity>> boxEntites;
			ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntites);
			for (const auto& entity : boxEntites)
			{
				auto& box = entity->GetComponent<BoxComponent2D>();

				if (entity->GetComponentFlag<BoxComponent2D, FlagDirty>())
				{
					InitBox2D(entity);

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
					InitCircle2D(entity);

					entity->SetComponentFlag<CircleComponent2D, FlagDirty>(false);
				}
			}
		}

		void Onager2DPhysicsSystem::Update()
		{
			m_collidersUpdated = false;

			UpdateComponents();
			Step();
		}

		void Onager2DPhysicsSystem::Stop()
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
				CleanupBox2D(entity);
			}

			std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
			ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
			for (const auto& entity : circleEntities)
			{
				CleanupCircle2D(entity);
			}
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::InitCircle2D(std::shared_ptr<ECS::Entity> entity)
		{
			auto& circle = entity->GetComponent<CircleComponent2D>();

			// If there is no shape for this entity in vector
			if (!m_circleShapes.Contains(entity->ID()))
			{
				m_circleShapes.Emplace(entity->ID(), CircleShape2D());
			}

			m_circleShapes[entity->ID()].centreOfMass = circle.centreOfMass;
			m_circleShapes[entity->ID()].radius = circle.radius;
			
			// If there is no collider for this entity, create new one
			auto collider = std::make_shared<Collision2D::CircleCollider2D>(entity->ID(), &m_circleShapes[entity->ID()]);

			InsertCollider(entity->ID(), collider);
		}

		void Onager2DPhysicsSystem::InitBox2D(std::shared_ptr<ECS::Entity> entity)
		{
			auto& box = entity->GetComponent<BoxComponent2D>();

			// If there is no shape for this entity in vector
			if (!m_boxShapes.Contains(entity->ID()))
			{
				m_boxShapes.Emplace(entity->ID(), BoxShape2D());
			}

			m_boxShapes[entity->ID()].centreOfMass = box.centreOfMass;
			m_boxShapes[entity->ID()].halfExtent = box.halfExtent;
			m_boxShapes[entity->ID()].UpdatePoints();

			auto collider = std::make_shared<Collision2D::BoxCollider2D>(entity->ID(), &m_boxShapes[entity->ID()]);

			InsertCollider(entity->ID(), collider);
		}

		void Onager2DPhysicsSystem::CleanupCircle2D(std::shared_ptr<ECS::Entity> entity)
		{
			if (m_circleShapes.Contains(entity->ID()))
				m_circleShapes.Erase(entity->ID());

			EraseCollider(entity->ID());
		}

		void Onager2DPhysicsSystem::CleanupBox2D(std::shared_ptr<ECS::Entity> entity)
		{
			if (m_boxShapes.Contains(entity->ID()))
				m_boxShapes.Erase(entity->ID());

			EraseCollider(entity->ID());
		}

		void Onager2DPhysicsSystem::UpdateComponents()
		{
			std::vector<std::shared_ptr<ECS::Entity>> rigidbodyEntities;
			ECS::GetEntities<TransformComponent, RigidbodyComponent2D>(m_world, rigidbodyEntities);
			for (const auto& entity : rigidbodyEntities)
			{
				if (entity->GetComponentFlag<RigidbodyComponent2D, FlagDeleted>())
				{
					entity->RemoveComponent<RigidbodyComponent2D>();
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
					InitBox2D(entity);

					entity->SetComponentFlag<BoxComponent2D, FlagDirty>(false);
				}

				// Cleanup Boxes
				if (entity->GetComponentFlag<BoxComponent2D, FlagDeleted>())
				{
					CleanupBox2D(entity);

					entity->RemoveComponent<BoxComponent2D>();
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
					InitCircle2D(entity);

					entity->SetComponentFlag<CircleComponent2D, FlagDirty>(false);
				}

				// Cleanup Circles
				if (entity->GetComponentFlag<CircleComponent2D, FlagDeleted>())
				{
					CleanupCircle2D(entity);

					entity->RemoveComponent<CircleComponent2D>();
				}
			}
		}

		void Onager2DPhysicsSystem::InsertCollider(ECS::EntityID id, std::shared_ptr<Collision2D::Collider2D> collider)
		{
			if (!m_colliders.Contains(id))
			{
				m_colliders.Insert(id, collider);

				m_collidersUpdated = true;
			}
		}

		void Onager2DPhysicsSystem::EraseCollider(ECS::EntityID id)
		{
			if (m_colliders.Contains(id))
			{
				m_colliders.Erase(id);

				m_collidersUpdated = true;
			}
		}

		void Onager2DPhysicsSystem::Step()
		{
			// Update Dynamic Objects
			UpdateDynamics();

			// Copy component transform into collider
			for (auto collider : m_colliders)
			{
				const auto& transform = m_world->GetComponent<TransformComponent>(collider->entity);

				collider->position = transform.position.GetXY();
				collider->rotation = transform.rotation.EulerAnglesDeg().z;
			}

			// Perform Collision2D Broadphase to check if two Colliders can collide
			CollisionBroadphase();

			// Check for Collision2D between Colliders
			CollisionDetection();

			// Resolve Collision2D between Colliders
			CollisionResponse();

			// Generate Collision Events for this Tick
			GenerateCollisionEvents();
		}

		void Onager2DPhysicsSystem::UpdateDynamics() const
		{
			std::vector<std::shared_ptr<ECS::Entity>> rigidbodyEntities;
			ECS::GetEntities<TransformComponent, VelocityComponent, RigidbodyComponent2D>(m_world, rigidbodyEntities);
			for (const auto& entity : rigidbodyEntities)
			{
				auto& transform = entity->GetComponent<TransformComponent>();
				auto& rigidbody = entity->GetComponent<RigidbodyComponent2D>();
				auto& velocity = entity->GetComponent<VelocityComponent>();

				// If a body has no mass, then it is kinematic/static and should not experience forces
				if (rigidbody.invMass == 0.0f || rigidbody.bodyType != BodyType::Dynamic)
					continue;

				CalculateImpulseByGravity(rigidbody);

				// Update Position
				transform.position += rigidbody.linearVelocity * m_engine->GetTimeStep();

				Vector3f euler = transform.rotation.EulerAnglesDeg();

				// Update Rotation
				//euler.z += rigidbody.angularVelocity * m_engine->GetTimeStep();

				if (euler.z > 360.0f)
				{
					euler.z = 0.0f;
				}

				transform.rotation = Maths::Quat::FromEulerAngles(euler.x, euler.y, euler.z);

				velocity.linear.x = rigidbody.linearVelocity.x;
				velocity.linear.y = rigidbody.linearVelocity.y;
				velocity.angular.z = rigidbody.angularVelocity;
			}
		}

		void Onager2DPhysicsSystem::CalculateImpulseByGravity(RigidbodyComponent2D& body) const
		{
			if (body.invMass == 0.0f)
				return;

			float mass = 1.0f / body.invMass;
			Vector2 impulseGravity = m_gravity * mass * m_engine->GetTimeStep();
			
			ApplyLinearImpulse(body, impulseGravity);
		}

		void Onager2DPhysicsSystem::CollisionBroadphase()
		{
			// Perform Collision Broadphase to Generate Collision Pairs
			if (m_activeBroadphase)
				m_activeBroadphase->GenerateCollisionPairs(m_colliders, m_collisionPairs, m_collidersUpdated);
		}

		void Onager2DPhysicsSystem::CollisionDetection()
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

		void Onager2DPhysicsSystem::CollisionResponse() const
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

					const Vector2 ds = (contact.pointOnB - contact.pointOnA) * contact.normal.Abs();
					transformA.position += ds * tA;
					transformB.position -= ds * tB;
				}
			}
		}

		void Onager2DPhysicsSystem::GenerateCollisionEvents()
		{
			auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

			std::set<Collision2D::Contact> existingContacts;

			// Iterate over contacts for this tick
			for (const Collision2D::Contact& contact : m_collisionContacts)
			{
				// Publish Begin Collision Event when a contact was not in active contacts set
				if (m_activeContacts.count(contact) == 0)
				{
					m_activeContacts.insert(contact);

					eventSubsystem->Publish<CollisionBeginEvent>({ contact.a, contact.b });
				}
				else
				{
					existingContacts.insert(contact);
				}
			}

			std::set<Collision2D::Contact> collisionsToRemove;

			// Iterate over contacts that were already in active set
			for (const Collision2D::Contact& contact : existingContacts)
			{
				// Publish End 
				if (existingContacts.count(contact) == 0)
				{
					collisionsToRemove.insert(contact);
				}
			}

			// Remove inactive contacts from active set and publish End Collision Event
			for (const Collision2D::Contact& contact : collisionsToRemove)
			{
				m_activeContacts.erase(contact);

				eventSubsystem->Publish<CollisionEndEvent>({ contact.a, contact.b });
			}
		}
	}
}

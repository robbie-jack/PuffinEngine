
#include "Physics/Onager2D/Onager2DPhysicsSystem.h"

#include <ECS/EnTTSubsystem.hpp>

#include "Physics/CollisionEvent.h"
#include "Components/TransformComponent.h"
#include "Components/Physics/VelocityComponent.hpp"
#include "Types/ComponentFlags.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Engine/Engine.hpp"
#include "ECS/Entity.hpp"
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
			m_boxShapes.Reserve(6000);
			m_circleShapes.Reserve(2000);
			m_colliders.Reserve(6000);

			m_systemInfo.name = "Onager2DPhysicsSystem";
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::Init()
		{
			auto eventSubsystem = m_engine->getSubsystem<Core::EventSubsystem>();

			// Register Events
			eventSubsystem->RegisterEvent<CollisionBeginEvent>();
			eventSubsystem->RegisterEvent<CollisionEndEvent>();

			RegisterBroadphase<NSquaredBroadphase>();
			RegisterBroadphase<SweepAndPruneBroadphase>();
			RegisterBroadphase<SpatialHashBroadphase2D>();

			SetBroadphase<SweepAndPruneBroadphase>();
		}

		void Onager2DPhysicsSystem::FixedUpdate()
		{
			// Update Dynamic Objects
			UpdateDynamics();

			auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			// Copy component transform into collider
			for (auto collider : m_colliders)
			{
				//const auto& transform = m_world->GetComponent<TransformComponent>(collider->entity);
				const auto& transform = registry->get<const TransformComponent>(m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(collider->uuid));

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

			m_collidersUpdated = false;
		}

		void Onager2DPhysicsSystem::Stop()
		{
			m_boxShapes.Clear();
			m_circleShapes.Clear();
			m_colliders.Clear();
			m_collisionPairs.clear();
			m_collisionContacts.clear();
		}

		void Onager2DPhysicsSystem::OnConstructBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& box = registry.get<const BoxComponent2D>(entity);

			InitBox(entity, object, box);
		}

		void Onager2DPhysicsSystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			CleanupBox(object);
		}

		void Onager2DPhysicsSystem::OnConstructCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& circle = registry.get<const CircleComponent2D>(entity);

			InitCircle(entity, object, circle);
		}

		void Onager2DPhysicsSystem::OnDestroyCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			CleanupCircle(object);
		}

		void Onager2DPhysicsSystem::OnConstructRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			if (m_shapes.Contains(object.uuid))
			{
				auto collider = std::make_shared<Collision2D::BoxCollider2D>(object.uuid, &m_boxShapes[object.uuid]);

				InsertCollider(object.uuid, collider);
			}
		}

		void Onager2DPhysicsSystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			EraseCollider(object.uuid);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::InitCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
			if (!m_circleShapes.Contains(object.uuid))
			{
				m_circleShapes.Emplace(object.uuid, CircleShape2D());
			}

			m_circleShapes[object.uuid].centreOfMass = circle.centreOfMass;
			m_circleShapes[object.uuid].radius = circle.radius;

			if (m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
				auto collider = std::make_shared<Collision2D::CircleCollider2D>(object.uuid, &m_circleShapes[object.uuid]);

				InsertCollider(object.uuid, collider);
			}
		}

		void Onager2DPhysicsSystem::CleanupCircle(const SceneObjectComponent& object)
		{
			if (m_circleShapes.Contains(object.uuid))
			{
				m_circleShapes.Erase(object.uuid);
				m_shapes.Erase(object.uuid);
			}

			EraseCollider(object.uuid);
		}

		void Onager2DPhysicsSystem::InitBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
			if (!m_boxShapes.Contains(object.uuid))
			{
				m_boxShapes.Emplace(object.uuid, BoxShape2D());
			}

			m_boxShapes[object.uuid].centreOfMass = box.centreOfMass;
			m_boxShapes[object.uuid].halfExtent = box.halfExtent;
			m_boxShapes[object.uuid].UpdatePoints();

			if (!m_shapes.Contains(object.uuid))
			{
				m_shapes.Emplace(object.uuid, nullptr);

				m_shapes[object.uuid] = &m_boxShapes[object.uuid];

				if (m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity) && !m_colliders.Contains(object.uuid))
				{
					auto collider = std::make_shared<Collision2D::BoxCollider2D>(object.uuid, &m_boxShapes[object.uuid]);

					InsertCollider(object.uuid, collider);
				}
			}
		}

		void Onager2DPhysicsSystem::CleanupBox(const SceneObjectComponent& object)
		{
			if (m_boxShapes.Contains(object.uuid))
				m_boxShapes.Erase(object.uuid);

			EraseCollider(object.uuid);
		}

		void Onager2DPhysicsSystem::InsertCollider(UUID id, std::shared_ptr<Collision2D::Collider2D> collider)
		{
			if (!m_colliders.Contains(id))
			{
				m_colliders.Insert(id, collider);

				m_collidersUpdated = true;
			}
		}

		void Onager2DPhysicsSystem::EraseCollider(UUID id)
		{
			if (m_colliders.Contains(id))
			{
				m_colliders.Erase(id);

				m_collidersUpdated = true;
			}
		}

		void Onager2DPhysicsSystem::UpdateDynamics() const
		{
			auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			auto rbView = registry->view<TransformComponent, RigidbodyComponent2D, VelocityComponent>();

			for (auto [entity, transform, rb, velocity] : rbView.each())
			{
				// If a body has no mass, then it is kinematic/static and should not experience forces
				if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
					continue;

				CalculateImpulseByGravity(rb);

				// Update Position
				transform.position += rb.linearVelocity * m_engine->timeStepFixed();

				Vector3f euler = transform.rotation.EulerAnglesDeg();

				// Update Rotation
				//euler.z += rb.angularVelocity * m_engine->GetTimeStep();

				if (euler.z > 360.0f)
				{
					euler.z = 0.0f;
				}

				transform.rotation = Maths::Quat::FromEulerAngles(euler.x, euler.y, euler.z);

				velocity.linear.x = rb.linearVelocity.x;
				velocity.linear.y = rb.linearVelocity.y;
				velocity.angular.z = rb.angularVelocity;
			}
		}

		void Onager2DPhysicsSystem::CalculateImpulseByGravity(RigidbodyComponent2D& body) const
		{
			if (body.mass == 0.0f)
				return;

			float mass = 1.0f / body.mass;
			Vector2 impulseGravity = m_gravity * mass * m_engine->timeStepFixed();
			
			ApplyLinearImpulse(body, impulseGravity);
		}

		void Onager2DPhysicsSystem::CollisionBroadphase()
		{
			m_collisionPairs.clear();
			m_collisionPairs.reserve(m_colliders.Size() * m_colliders.Size());

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
			auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			for (const Collision2D::Contact& contact : m_collisionContacts)
			{
				auto entityA = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.a);
				auto entityB = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.b);

				auto& transformA = registry->get<TransformComponent>(entityA);
				auto& transformB = registry->get<TransformComponent>(entityB);

				if (registry->all_of<RigidbodyComponent2D>(entityA) && registry->all_of<RigidbodyComponent2D>(entityB))
				{
					auto& bodyA = registry->get<RigidbodyComponent2D>(entityA);
					auto& bodyB = registry->get<RigidbodyComponent2D>(entityB);

					const float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const float nVab = vab.Dot(contact.normal);

					/*if (nVab <= 0)
						continue;*/

					const float impulseJ = -(1.0f + elasticity) * nVab / (bodyA.mass + bodyB.mass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					// Apply Impulse to body A
					ApplyImpulse(bodyA, contact.pointOnA, vectorImpulseJ * 1.0f);

					// Apply Impulse to body B
					ApplyImpulse(bodyB, contact.pointOnB, vectorImpulseJ * -1.0f);

					// Calculate impulse caused by friction


					// Move colliding bodies to just outside each other
					const float tA = bodyA.mass / (bodyA.mass + bodyB.mass);
					const float tB = bodyB.mass / (bodyA.mass + bodyB.mass);

					const Vector2 ds = (contact.pointOnB - contact.pointOnA) * contact.normal.Abs();
					transformA.position += ds * tA;
					transformB.position -= ds * tB;
				}
			}
		}

		void Onager2DPhysicsSystem::GenerateCollisionEvents()
		{
			auto eventSubsystem = m_engine->getSubsystem<Core::EventSubsystem>();

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

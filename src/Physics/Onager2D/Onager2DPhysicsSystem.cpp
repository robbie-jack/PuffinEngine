
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

namespace puffin
{
	namespace physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		Onager2DPhysicsSystem::Onager2DPhysicsSystem()
		{
			boxShapes_.Reserve(6000);
			circleShapes_.Reserve(2000);
			colliders_.Reserve(6000);

			m_systemInfo.name = "Onager2DPhysicsSystem";
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::init()
		{
			const auto eventSubsystem = m_engine->getSubsystem<core::EventSubsystem>();

			// Register Events
			eventSubsystem->RegisterEvent<CollisionBeginEvent>();
			eventSubsystem->RegisterEvent<CollisionEndEvent>();

			registerBroadphase<NSquaredBroadphase>();
			registerBroadphase<SweepAndPruneBroadphase>();
			registerBroadphase<SpatialHashBroadphase2D>();

			setBroadphase<SweepAndPruneBroadphase>();
		}

		void Onager2DPhysicsSystem::fixedUpdate()
		{
			// Update Dynamic Objects
			updateDynamics();

			const auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			// Copy component transform into collider
			for (const auto collider : colliders_)
			{
				//const auto& transform = m_world->GetComponent<TransformComponent>(collider->entity);
				const auto& transform = registry->get<const TransformComponent>(m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(collider->uuid));

				collider->position = transform.position.GetXY();
				collider->rotation = transform.rotation.EulerAnglesDeg().z;
			}

			// Perform Collision2D Broadphase to check if two Colliders can collide
			collisionBroadphase();

			// Check for Collision2D between Colliders
			collisionDetection();

			// Resolve Collision2D between Colliders
			collisionResponse();

			// Generate Collision Events for this Tick
			generateCollisionEvents();

			collidersUpdated_ = false;
		}

		void Onager2DPhysicsSystem::stop()
		{
			boxShapes_.Clear();
			circleShapes_.Clear();
			colliders_.Clear();
			collisionPairs_.clear();
			collisionContacts_.clear();
		}

		void Onager2DPhysicsSystem::onConstructBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& box = registry.get<const BoxComponent2D>(entity);

			initBox(entity, object, box);
		}

		void Onager2DPhysicsSystem::onDestroyBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			cleanupBox(object);
		}

		void Onager2DPhysicsSystem::onConstructCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& circle = registry.get<const CircleComponent2D>(entity);

			initCircle(entity, object, circle);
		}

		void Onager2DPhysicsSystem::onDestroyCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			cleanupCircle(object);
		}

		void Onager2DPhysicsSystem::onConstructRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			if (shapes_.Contains(object.uuid))
			{
				const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.uuid, &boxShapes_[object.uuid]);

				insertCollider(object.uuid, collider);
			}
		}

		void Onager2DPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			eraseCollider(object.uuid);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void Onager2DPhysicsSystem::initCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
			if (!circleShapes_.Contains(object.uuid))
			{
				circleShapes_.Emplace(object.uuid, CircleShape2D());
			}

			circleShapes_[object.uuid].centreOfMass = circle.centreOfMass;
			circleShapes_[object.uuid].radius = circle.radius;

			if (m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
				const auto collider = std::make_shared<collision2D::CircleCollider2D>(object.uuid, &circleShapes_[object.uuid]);

				insertCollider(object.uuid, collider);
			}
		}

		void Onager2DPhysicsSystem::cleanupCircle(const SceneObjectComponent& object)
		{
			if (circleShapes_.Contains(object.uuid))
			{
				circleShapes_.Erase(object.uuid);
				shapes_.Erase(object.uuid);
			}

			eraseCollider(object.uuid);
		}

		void Onager2DPhysicsSystem::initBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
			if (!boxShapes_.Contains(object.uuid))
			{
				boxShapes_.Emplace(object.uuid, BoxShape2D());
			}

			boxShapes_[object.uuid].centreOfMass = box.centreOfMass;
			boxShapes_[object.uuid].halfExtent = box.halfExtent;
			boxShapes_[object.uuid].updatePoints();

			if (!shapes_.Contains(object.uuid))
			{
				shapes_.Emplace(object.uuid, nullptr);

				shapes_[object.uuid] = &boxShapes_[object.uuid];

				if (m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity) && !colliders_.Contains(object.uuid))
				{
					const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.uuid, &boxShapes_[object.uuid]);

					insertCollider(object.uuid, collider);
				}
			}
		}

		void Onager2DPhysicsSystem::cleanupBox(const SceneObjectComponent& object)
		{
			if (boxShapes_.Contains(object.uuid))
				boxShapes_.Erase(object.uuid);

			eraseCollider(object.uuid);
		}

		void Onager2DPhysicsSystem::insertCollider(UUID id, std::shared_ptr<collision2D::Collider2D> collider)
		{
			if (!colliders_.Contains(id))
			{
				colliders_.Insert(id, collider);

				collidersUpdated_ = true;
			}
		}

		void Onager2DPhysicsSystem::eraseCollider(UUID id)
		{
			if (colliders_.Contains(id))
			{
				colliders_.Erase(id);

				collidersUpdated_ = true;
			}
		}

		void Onager2DPhysicsSystem::updateDynamics() const
		{
			const auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			const auto rbView = registry->view<TransformComponent, RigidbodyComponent2D, VelocityComponent>();

			for (auto [entity, transform, rb, velocity] : rbView.each())
			{
				// If a body has no mass, then it is kinematic/static and should not experience forces
				if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
					continue;

				calculateImpulseByGravity(rb);

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

		void Onager2DPhysicsSystem::calculateImpulseByGravity(RigidbodyComponent2D& body) const
		{
			if (body.mass == 0.0f)
				return;

			const float mass = 1.0f / body.mass;
			const Vector2 impulseGravity = gravity_ * mass * m_engine->timeStepFixed();
			
			applyLinearImpulse(body, impulseGravity);
		}

		void Onager2DPhysicsSystem::collisionBroadphase()
		{
			collisionPairs_.clear();
			collisionPairs_.reserve(colliders_.Size() * colliders_.Size());

			// Perform Collision Broadphase to Generate Collision Pairs
			if (activeBroadphase_)
				activeBroadphase_->generateCollisionPairs(colliders_, collisionPairs_, collidersUpdated_);
		}

		void Onager2DPhysicsSystem::collisionDetection()
		{
			collisionContacts_.clear();

			for (const CollisionPair& collisionPair : collisionPairs_)
			{
				collision2D::Contact contact;

				// Put collision detection here
				if (collisionPair.first->testCollision(collisionPair.second.get(), contact))
				{
					collisionContacts_.emplace_back(contact);
				}
			}
		}

		void Onager2DPhysicsSystem::collisionResponse() const
		{
			const auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			for (const collision2D::Contact& contact : collisionContacts_)
			{
				const auto entityA = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.a);
				const auto entityB = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.b);

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
					applyImpulse(bodyA, contact.pointOnA, vectorImpulseJ * 1.0f);

					// Apply Impulse to body B
					applyImpulse(bodyB, contact.pointOnB, vectorImpulseJ * -1.0f);

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

		void Onager2DPhysicsSystem::generateCollisionEvents()
		{
			const auto eventSubsystem = m_engine->getSubsystem<core::EventSubsystem>();

			std::set<collision2D::Contact> existingContacts;

			// Iterate over contacts for this tick
			for (const collision2D::Contact& contact : collisionContacts_)
			{
				// Publish Begin Collision Event when a contact was not in active contacts set
				if (activeContacts_.count(contact) == 0)
				{
					activeContacts_.insert(contact);

					eventSubsystem->Publish<CollisionBeginEvent>({ contact.a, contact.b });
				}
				else
				{
					existingContacts.insert(contact);
				}
			}

			std::set<collision2D::Contact> collisionsToRemove;

			// Iterate over contacts that were already in active set
			for (const collision2D::Contact& contact : existingContacts)
			{
				// Publish End 
				if (existingContacts.count(contact) == 0)
				{
					collisionsToRemove.insert(contact);
				}
			}

			// Remove inactive contacts from active set and publish End Collision Event
			for (const collision2D::Contact& contact : collisionsToRemove)
			{
				activeContacts_.erase(contact);

				eventSubsystem->Publish<CollisionEndEvent>({ contact.a, contact.b });
			}
		}
	}
}

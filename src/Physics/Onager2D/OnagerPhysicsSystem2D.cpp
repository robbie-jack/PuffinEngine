
#include "Physics/Onager2D/OnagerPhysicsSystem2D.h"

#include "Components/TransformComponent.h"
#include "Components\Physics\VelocityComponent.h"
#include "ECS/EnTTSubsystem.h"
#include "Engine\Engine.h"
#include "Engine\SignalSubsystem.h"
#include "Physics/CollisionEvent.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Physics\Onager2D\Broadphases\SpatialHashBroadphase2D.h"
#include "Physics\Onager2D\Broadphases\SweepAndPruneBroadphase.h"

namespace puffin
{
	namespace physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		OnagerPhysicsSystem2D::OnagerPhysicsSystem2D()
		{
			mBoxShapes.Reserve(6000);
			mCircleShapes.Reserve(2000);
			mColliders.Reserve(6000);

			mSystemInfo.name = "Onager2DPhysicsSystem";
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void OnagerPhysicsSystem2D::init()
		{
			registerBroadphase<NSquaredBroadphase>();
			registerBroadphase<SweepAndPruneBroadphase>();
			registerBroadphase<SpatialHashBroadphase2D>();

			setBroadphase<SweepAndPruneBroadphase>();
		}

		void OnagerPhysicsSystem2D::fixedUpdate()
		{
			// Update Dynamic Objects
			updateDynamics();

			const auto registry = mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			// Copy component transform into collider
			for (const auto collider : mColliders)
			{
				//const auto& transform = m_world->GetComponent<TransformComponent>(collider->entity);
				const auto& transform = registry->get<const TransformComponent>(mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(collider->uuid));

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

			mCollidersUpdated = false;
		}

		void OnagerPhysicsSystem2D::stop()
		{
			mBoxShapes.Clear();
			mCircleShapes.Clear();
			mColliders.Clear();
			mCollisionPairs.clear();
			mCollisionContacts.clear();
		}

		void OnagerPhysicsSystem2D::onConstructBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& box = registry.get<const BoxComponent2D>(entity);

			initBox(entity, object, box);
		}

		void OnagerPhysicsSystem2D::onDestroyBox(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			cleanupBox(object);
		}

		void OnagerPhysicsSystem2D::onConstructCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);
			const auto& circle = registry.get<const CircleComponent2D>(entity);

			initCircle(entity, object, circle);
		}

		void OnagerPhysicsSystem2D::onDestroyCircle(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			cleanupCircle(object);
		}

		void OnagerPhysicsSystem2D::onConstructRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			if (mShapes.Contains(object.uuid))
			{
				const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.uuid, &mBoxShapes[object.uuid]);

				insertCollider(object.uuid, collider);
			}
		}

		void OnagerPhysicsSystem2D::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			eraseCollider(object.uuid);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void OnagerPhysicsSystem2D::initCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
			if (!mCircleShapes.Contains(object.uuid))
			{
				mCircleShapes.Emplace(object.uuid, CircleShape2D());
			}

			mCircleShapes[object.uuid].centreOfMass = circle.centreOfMass;
			mCircleShapes[object.uuid].radius = circle.radius;

			if (mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
				const auto collider = std::make_shared<collision2D::CircleCollider2D>(object.uuid, &mCircleShapes[object.uuid]);

				insertCollider(object.uuid, collider);
			}
		}

		void OnagerPhysicsSystem2D::cleanupCircle(const SceneObjectComponent& object)
		{
			if (mCircleShapes.Contains(object.uuid))
			{
				mCircleShapes.Erase(object.uuid);
				mShapes.Erase(object.uuid);
			}

			eraseCollider(object.uuid);
		}

		void OnagerPhysicsSystem2D::initBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
			if (!mBoxShapes.Contains(object.uuid))
			{
				mBoxShapes.Emplace(object.uuid, BoxShape2D());
			}

			mBoxShapes[object.uuid].centreOfMass = box.centreOfMass;
			mBoxShapes[object.uuid].halfExtent = box.halfExtent;
			mBoxShapes[object.uuid].updatePoints();

			if (!mShapes.Contains(object.uuid))
			{
				mShapes.Emplace(object.uuid, nullptr);

				mShapes[object.uuid] = &mBoxShapes[object.uuid];

				if (mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry()->all_of<RigidbodyComponent2D>(entity) && !mColliders.Contains(object.uuid))
				{
					const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.uuid, &mBoxShapes[object.uuid]);

					insertCollider(object.uuid, collider);
				}
			}
		}

		void OnagerPhysicsSystem2D::cleanupBox(const SceneObjectComponent& object)
		{
			if (mBoxShapes.Contains(object.uuid))
				mBoxShapes.Erase(object.uuid);

			eraseCollider(object.uuid);
		}

		void OnagerPhysicsSystem2D::insertCollider(UUID id, std::shared_ptr<collision2D::Collider2D> collider)
		{
			if (!mColliders.Contains(id))
			{
				mColliders.Insert(id, collider);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSystem2D::eraseCollider(UUID id)
		{
			if (mColliders.Contains(id))
			{
				mColliders.Erase(id);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSystem2D::updateDynamics() const
		{
			const auto registry = mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			const auto rbView = registry->view<TransformComponent, RigidbodyComponent2D, VelocityComponent>();

			for (auto [entity, transform, rb, velocity] : rbView.each())
			{
				// If a body has no mass, then it is kinematic/static and should not experience forces
				if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
					continue;

				calculateImpulseByGravity(rb);

				// Update Position
				transform.position += rb.linearVelocity * mEngine->timeStepFixed();

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

		void OnagerPhysicsSystem2D::calculateImpulseByGravity(RigidbodyComponent2D& body) const
		{
			if (body.mass == 0.0f)
				return;

			const float mass = 1.0f / body.mass;
			const Vector2 impulseGravity = mGravity * mass * mEngine->timeStepFixed();
			
			applyLinearImpulse(body, impulseGravity);
		}

		void OnagerPhysicsSystem2D::collisionBroadphase()
		{
			mCollisionPairs.clear();
			mCollisionPairs.reserve(mColliders.Size() * mColliders.Size());

			// Perform Collision Broadphase to Generate Collision Pairs
			if (mActiveBroadphase)
				mActiveBroadphase->generateCollisionPairs(mColliders, mCollisionPairs, mCollidersUpdated);
		}

		void OnagerPhysicsSystem2D::collisionDetection()
		{
			mCollisionContacts.clear();

			for (const CollisionPair& collisionPair : mCollisionPairs)
			{
				collision2D::Contact contact;

				// Put collision detection here
				if (collisionPair.first->testCollision(collisionPair.second.get(), contact))
				{
					mCollisionContacts.emplace_back(contact);
				}
			}
		}

		void OnagerPhysicsSystem2D::collisionResponse() const
		{
			const auto registry = mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			for (const collision2D::Contact& contact : mCollisionContacts)
			{
				const auto entityA = mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.a);
				const auto entityB = mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(contact.b);

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

		void OnagerPhysicsSystem2D::generateCollisionEvents()
		{
			const auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

			std::set<collision2D::Contact> existingContacts;

			// Iterate over contacts for this tick
			for (const collision2D::Contact& contact : mCollisionContacts)
			{
				// Publish Begin Collision Event when a contact was not in active contacts set
				if (mActiveContacts.count(contact) == 0)
				{
					mActiveContacts.insert(contact);

					signalSubsystem->signal<CollisionBeginEvent>({ contact.a, contact.b });
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
				mActiveContacts.erase(contact);

				signalSubsystem->signal<CollisionEndEvent>({ contact.a, contact.b });
			}
		}
	}
}


#include "Physics/Onager2D/OnagerPhysicsSystem2D.h"

#if PFN_ONAGER2D_PHYSICS

#include "Components/TransformComponent2D.h"
#include "Components/Physics/2D/VelocityComponent2D.h"
#include "Core/Engine.h"
#include "Core/EnkiTSSubsystem.h"
#include "Core/SignalSubsystem.h"
#include "puffin/ecs/entt_subsystem.h"
#include "Physics/CollisionEvent.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.h"
#include "Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.h"

namespace puffin
{
	namespace physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		OnagerPhysicsSystem2D::OnagerPhysicsSystem2D(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			mEngine->registerCallback(core::ExecutionStage::Startup, [&]() { startup(); }, "Onager2DPhysicsSystem: Startup");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&]() { fixedUpdate(); }, "Onager2DPhysicsSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&]() { endPlay(); }, "Onager2DPhysicsSystem: EndPlay");

			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent2D>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent2D>>();

			registry->on_construct<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructBox>(this);
			registry->on_update<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructCircle>(this);
			registry->on_update<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyCircle>(this);

			const auto signalSubsystem = mEngine->getSystem<core::SignalSubsystem>();
			signalSubsystem->createSignal<CollisionBeginEvent>("CollisionBegin");
			signalSubsystem->createSignal<CollisionEndEvent>("CollisionEnd");

			mBoxShapes.reserve(20000);
			mCircleShapes.reserve(20000);
			mColliders.reserve(40000);
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void OnagerPhysicsSystem2D::startup()
		{
			registerBroadphase<NSquaredBroadphase>();
			registerBroadphase<SweepAndPruneBroadphase>();
			registerBroadphase<SpatialHashBroadphase2D>();

			setBroadphase<SpatialHashBroadphase2D>();
		}

		void OnagerPhysicsSystem2D::fixedUpdate()
		{
			// Update Dynamic Objects
			updateDynamics();

			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			// Copy component transform into collider
			for (const auto collider : mColliders)
			{
				const auto& transform = registry->get<const TransformComponent2D>(mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(collider->uuid));

				collider->position = transform.position;
				//collider->rotation = maths::RadiansToDegrees(transform.orientation.toEulerAngles().z);
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

		void OnagerPhysicsSystem2D::endPlay()
		{
			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			registry->clear<BoxComponent2D>();
			registry->clear<CircleComponent2D>();
			registry->clear<RigidbodyComponent2D>();

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

			if (mShapes.contains(object.id))
			{
				const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.id, &mBoxShapes[object.id]);

				insertCollider(object.id, collider);
			}
		}

		void OnagerPhysicsSystem2D::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
			const auto& object = registry.get<const SceneObjectComponent>(entity);

			eraseCollider(object.id);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

		void OnagerPhysicsSystem2D::initCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
			if (!mCircleShapes.contains(object.id))
			{
				mCircleShapes.emplace(object.id, CircleShape2D());
			}

			mCircleShapes[object.id].centreOfMass = circle.centreOfMass;
			mCircleShapes[object.id].radius = circle.radius;

			if (mEngine->getSystem<ecs::EnTTSubsystem>()->registry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
				const auto collider = std::make_shared<collision2D::CircleCollider2D>(object.id, &mCircleShapes[object.id]);

				insertCollider(object.id, collider);
			}
		}

		void OnagerPhysicsSystem2D::cleanupCircle(const SceneObjectComponent& object)
		{
			if (mCircleShapes.contains(object.id))
			{
				mCircleShapes.erase(object.id);
				mShapes.erase(object.id);
			}

			eraseCollider(object.id);
		}

		void OnagerPhysicsSystem2D::initBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
			if (!mBoxShapes.contains(object.id))
			{
				mBoxShapes.emplace(object.id, BoxShape2D());
			}

			mBoxShapes[object.id].centreOfMass = box.centreOfMass;
			mBoxShapes[object.id].halfExtent = box.halfExtent;
			mBoxShapes[object.id].updatePoints();

			if (!mShapes.contains(object.id))
			{
				mShapes.emplace(object.id, nullptr);

				mShapes[object.id] = &mBoxShapes[object.id];

				if (mEngine->getSystem<ecs::EnTTSubsystem>()->registry()->all_of<RigidbodyComponent2D>(entity) && !mColliders.contains(object.id))
				{
					const auto collider = std::make_shared<collision2D::BoxCollider2D>(object.id, &mBoxShapes[object.id]);

					insertCollider(object.id, collider);
				}
			}
		}

		void OnagerPhysicsSystem2D::cleanupBox(const SceneObjectComponent& object)
		{
			if (mBoxShapes.contains(object.id))
				mBoxShapes.erase(object.id);

			eraseCollider(object.id);
		}

		void OnagerPhysicsSystem2D::insertCollider(PuffinID id, std::shared_ptr<collision2D::Collider2D> collider)
		{
			if (!mColliders.contains(id))
			{
				mColliders.insert(id, collider);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSystem2D::eraseCollider(PuffinID id)
		{
			if (mColliders.contains(id))
			{
				mColliders.erase(id);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSystem2D::updateDynamics() const
		{
			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			const auto enkiTSSubSystem = mEngine->getSystem<core::EnkiTSSubsystem>();

			const uint32_t numThreads = enkiTSSubSystem->getTaskScheduler()->GetNumTaskThreads();

			const auto rbView = registry->view<RigidbodyComponent2D>();
			const auto tView = registry->view<TransformComponent2D>();
			const auto vView = registry->view<VelocityComponent2D>();

			if (rbView.empty())
			{
				return;
			}

			// Apply gravity to rigibodies as impulse

			const int numRigidbodiesPerThread = std::ceil(rbView.size() / numThreads);

			enki::TaskSet gravityTask(rbView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t idx = range.start; idx < range.end; idx++)
				{
					const auto entity = rbView[idx];

					if (!registry->all_of<TransformComponent2D, VelocityComponent2D>(entity))
						continue;

					auto& rb = rbView.get<RigidbodyComponent2D>(entity);

					// If a body has no mass, then it is kinematic/static and should not experience forces
					if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
						continue;

					calculateImpulseByGravity(rb);
				}
			});

			enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&gravityTask);
			enkiTSSubSystem->getTaskScheduler()->WaitforTask(&gravityTask);

			// Apply velocity to transform

			std::vector<std::vector<entt::entity>> updatedEntities;
			updatedEntities.resize(numThreads);

			for (auto& entities : updatedEntities)
			{
				entities.reserve(std::max(10, numRigidbodiesPerThread));
			}

			enki::TaskSet transformTask(tView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t idx = range.start; idx < range.end; idx++)
				{
					const auto entity = rbView[idx];

					if (!registry->all_of<RigidbodyComponent2D, VelocityComponent2D>(entity))
						continue;

					auto& transform = tView.get<TransformComponent2D>(entity);
					const auto& rb = registry->get<RigidbodyComponent2D>(entity);

					// If a body has no mass, then it is kinematic/static and should not experience forces
					if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
						continue;

					// Update Position
					transform.position += rb.linearVelocity * mEngine->timeStepFixed();

					// Update Rotation
					transform.rotation += rb.angularVelocity * mEngine->timeStepFixed();

					updatedEntities[threadnum].emplace_back(entity);
				}
			});

			enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&transformTask);
			enkiTSSubSystem->getTaskScheduler()->WaitforTask(&transformTask);

			for (auto& entities : updatedEntities)
			{
				for (const auto& entity : entities)
				{
					registry->patch<TransformComponent2D>(entity, [](auto& transform) {});
				}

				entities.clear();
			}

			// Update velocity component

			enki::TaskSet velocityTask(vView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t idx = range.start; idx < range.end; idx++)
				{
					const auto entity = rbView[idx];

					if (!registry->all_of<RigidbodyComponent2D, TransformComponent2D>(entity))
						continue;

					auto& velocity = vView.get<VelocityComponent2D>(entity);
					const auto& rb = registry->get<RigidbodyComponent2D>(entity);

					// If a body has no mass, then it is kinematic/static and should not experience forces
					if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
						continue;

					velocity.linear.x = rb.linearVelocity.x;
					velocity.linear.y = rb.linearVelocity.y;
					velocity.angular = rb.angularVelocity;
				}
			});

			enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&velocityTask);
			enkiTSSubSystem->getTaskScheduler()->WaitforTask(&velocityTask);
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
			if (mColliders.size() == 0)
			{
				return;
			}

			mCollisionPairs.clear();
			mCollisionPairs.reserve(mColliders.size() * mColliders.size());

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
			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			for (const collision2D::Contact& contact : mCollisionContacts)
			{
				const auto entityA = mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(contact.a);
				const auto entityB = mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(contact.b);

				if (registry->all_of<RigidbodyComponent2D>(entityA) && registry->all_of<RigidbodyComponent2D>(entityB))
				{
					auto& bodyA = registry->get<RigidbodyComponent2D>(entityA);
					auto& bodyB = registry->get<RigidbodyComponent2D>(entityB);

					const float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
					const Vector2 vab = bodyA.linearVelocity - bodyB.linearVelocity;
					const float nVab = vab.dot(contact.normal);

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

					const Vector2 ds = (contact.pointOnB - contact.pointOnA) * contact.normal.abs();

					registry->patch<TransformComponent2D>(entityA, [&](auto& transform)
					{
						transform.position += ds * tA;
					});

					registry->patch<TransformComponent2D>(entityB, [&](auto& transform)
					{
						transform.position -= ds * tB;
					});
				}
			}
		}

		void OnagerPhysicsSystem2D::generateCollisionEvents()
		{
			const auto signalSubsystem = mEngine->getSystem<core::SignalSubsystem>();

			std::set<collision2D::Contact> existingContacts;

			// Iterate over contacts for this tick
			for (const collision2D::Contact& contact : mCollisionContacts)
			{
				// Publish Begin Collision Event when a contact was not in active contacts set
				if (mActiveContacts.count(contact) == 0)
				{
					mActiveContacts.insert(contact);

					signalSubsystem->emit<CollisionBeginEvent>("CollisionBegin", { contact.a, contact.b });
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

				signalSubsystem->emit<CollisionEndEvent>("CollisionEnd", { contact.a, contact.b });
			}
		}
	}
}

#endif // PFN_ONAGER2D_PHYSICS
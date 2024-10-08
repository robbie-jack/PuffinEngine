
#include "puffin/physics/onager2d/onager2dphysicssubsystem.h"

#if PFN_ONAGER2D_PHYSICS

#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/core/engine.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/physics/collisionevent.h"
#include "puffin/physics/onager2d/physicshelpers2d.h"
#include "puffin/physics/onager2d/broadphases/spatialhashbroadphase2d.h"
#include "puffin/physics/onager2d/broadphases/sweepandprunebroadphase2d.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/scene/scenegraph.h"

namespace puffin
{
	namespace physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		OnagerPhysicsSystem2D::OnagerPhysicsSystem2D(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

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

			const auto signalSubsystem = m_engine->get_system<core::SignalSubsystem>();
			signalSubsystem->create_signal<CollisionBeginEvent>("CollisionBegin");
			signalSubsystem->create_signal<CollisionEndEvent>("CollisionEnd");

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

		void OnagerPhysicsSystem2D::update_fixed()
		{
			// Update Dynamic Objects
			updateDynamics();

			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

			// Copy component transform into collider
			for (const auto collider : mColliders)
			{
                const auto& transform = registry->get<const TransformComponent2D>(m_engine->get_system<ecs::EnTTSubsystem>()->get_entity(collider->uuid));

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
			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

			registry->clear<BoxComponent2D>();
			registry->clear<CircleComponent2D>();
			registry->clear<RigidbodyComponent2D>();

			mCollisionPairs.clear();
			mCollisionContacts.clear();
		}

		void OnagerPhysicsSystem2D::onConstructBox(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);
			const auto& box = registry.get<const BoxComponent2D>(entity);

            initBox(entity, id, box);
		}

		void OnagerPhysicsSystem2D::onDestroyBox(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);

            cleanupBox(id);
		}

		void OnagerPhysicsSystem2D::onConstructCircle(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);
			const auto& circle = registry.get<const CircleComponent2D>(entity);

            initCircle(entity, id, circle);
		}

		void OnagerPhysicsSystem2D::onDestroyCircle(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);

            cleanupCircle(id);
		}

		void OnagerPhysicsSystem2D::onConstructRigidbody(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);

            if (mShapes.contains(id))
			{
                const auto collider = std::make_shared<collision2D::BoxCollider2D>(id, &mBoxShapes[id]);

                insertCollider(id, collider);
			}
		}

		void OnagerPhysicsSystem2D::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = m_engine->get_system<ecs::EnTTSubsystem>();
            const auto id = ecs_system->get_id(entity);

            eraseCollider(id);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

        void OnagerPhysicsSystem2D::initCircle(const entt::entity& entity, const puffin::PuffinID& id, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
            if (!mCircleShapes.contains(id))
			{
                mCircleShapes.emplace(id, CircleShape2D());
			}

            mCircleShapes[id].centre_of_mass = circle.centre_of_mass;
            mCircleShapes[id].radius = circle.radius;

			if (m_engine->get_system<ecs::EnTTSubsystem>()->registry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
                const auto collider = std::make_shared<collision2D::CircleCollider2D>(id, &mCircleShapes[id]);

                insertCollider(id, collider);
			}
		}

        void OnagerPhysicsSystem2D::cleanupCircle(const puffin::PuffinID& id)
		{
            if (mCircleShapes.contains(id))
			{
                mCircleShapes.erase(id);
                mShapes.erase(id);
			}

            eraseCollider(id);
		}

        void OnagerPhysicsSystem2D::initBox(const entt::entity& entity, const puffin::PuffinID& id, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
            if (!mBoxShapes.contains(id))
			{
                mBoxShapes.emplace(id, BoxShape2D());
			}

            mBoxShapes[id].centre_of_mass = box.centre_of_mass;
            mBoxShapes[id].half_extent = box.half_extent;
            mBoxShapes[id].updatePoints();

            if (!mShapes.contains(id))
			{
                mShapes.emplace(id, nullptr);

                mShapes[id] = &mBoxShapes[id];

                if (m_engine->get_system<ecs::EnTTSubsystem>()->registry()->all_of<RigidbodyComponent2D>(entity) && !mColliders.contains(id))
				{
                    const auto collider = std::make_shared<collision2D::BoxCollider2D>(id, &mBoxShapes[id]);

                    insertCollider(id, collider);
				}
			}
		}

        void OnagerPhysicsSystem2D::cleanupBox(const puffin::PuffinID& id)
		{
            if (mBoxShapes.contains(id))
                mBoxShapes.erase(id);

            eraseCollider(id);
		}

        void OnagerPhysicsSystem2D::insertCollider(const puffin::PuffinID& id, std::shared_ptr<collision2D::Collider2D> collider)
		{
			if (!mColliders.contains(id))
			{
                mColliders.emplace(id, collider);

				mCollidersUpdated = true;
			}
		}

        void OnagerPhysicsSystem2D::eraseCollider(const puffin::PuffinID& id)
		{
			if (mColliders.contains(id))
			{
				mColliders.erase(id);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSystem2D::updateDynamics() const
		{
			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

			const auto enkiTSSubSystem = m_engine->get_system<core::EnkiTSSubsystem>();

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
                    if (rb.mass == 0.0f || rb.body_type != BodyType::Dynamic)
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
                    if (rb.mass == 0.0f || rb.body_type != BodyType::Dynamic)
						continue;

					// Update Position
                    transform.position += rb.linear_velocity * m_engine->time_step_fixed();

					// Update Rotation
                    transform.rotation += rb.angular_velocity * m_engine->time_step_fixed();

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
                    if (rb.mass == 0.0f || rb.body_type != BodyType::Dynamic)
						continue;

                    velocity.linear.x = rb.linear_velocity.x;
                    velocity.linear.y = rb.linear_velocity.y;
                    velocity.angular = rb.angular_velocity;
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
			const Vector2 impulseGravity = mGravity * mass * m_engine->time_step_fixed();
			
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
			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

			for (const collision2D::Contact& contact : mCollisionContacts)
			{
                const auto entityA = m_engine->get_system<ecs::EnTTSubsystem>()->get_entity(contact.a);
                const auto entityB = m_engine->get_system<ecs::EnTTSubsystem>()->get_entity(contact.b);

				if (registry->all_of<RigidbodyComponent2D>(entityA) && registry->all_of<RigidbodyComponent2D>(entityB))
				{
					auto& bodyA = registry->get<RigidbodyComponent2D>(entityA);
					auto& bodyB = registry->get<RigidbodyComponent2D>(entityB);

					const float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
                    const Vector2 vab = bodyA.linear_velocity - bodyB.linear_velocity;
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
			const auto signalSubsystem = m_engine->get_system<core::SignalSubsystem>();

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

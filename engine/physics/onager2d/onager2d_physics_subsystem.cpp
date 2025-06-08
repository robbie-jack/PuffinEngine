
#include "physics/onager2d/onager2d_physics_subsystem.h"

#if PFN_ONAGER2D_PHYSICS

#include "component/transform_component_2d.h"
#include "component/physics/2d/velocity_component_2d.h"
#include "core/engine.h"
#include "core/enkits_subsystem.h"
#include "ecs/enttsubsystem.h"
#include "physics/collision_event.h"
#include "physics/onager2d/physics_helpers_2d.h"
#include "physics/onager2d/broadphases/spatial_hash_broadphase_2d.h"
#include "physics/onager2d/broadphases/sweep_and_prune_broadphase_2d.h"
#include "core/signal_subsystem.h"
#include "scene/scene_graph_subsystem.h"

namespace puffin
{
	namespace physics
	{
		//--------------------------------------------------
		// Constructor/Destructor
		//--------------------------------------------------

		OnagerPhysicsSubystem2D::OnagerPhysicsSubystem2D(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

			registry->on_construct<RigidbodyComponent2D>().connect<&OnagerPhysicsSubystem2D::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&OnagerPhysicsSubystem2D::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent2D>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent2D>>();

			registry->on_construct<BoxComponent2D>().connect<&OnagerPhysicsSubystem2D::onConstructBox>(this);
			registry->on_update<BoxComponent2D>().connect<&OnagerPhysicsSubystem2D::onConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&OnagerPhysicsSubystem2D::onDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&OnagerPhysicsSubystem2D::onConstructCircle>(this);
			registry->on_update<CircleComponent2D>().connect<&OnagerPhysicsSubystem2D::onConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&OnagerPhysicsSubystem2D::onDestroyCircle>(this);

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
			signalSubsystem->CreateSignal<CollisionBeginEvent>("CollisionBegin");
			signalSubsystem->CreateSignal<CollisionEndEvent>("CollisionEnd");

			mBoxShapes.Reserve(20000);
			mCircleShapes.Reserve(20000);
			mColliders.Reserve(40000);
		}

		//--------------------------------------------------
		// Public Functions
		//--------------------------------------------------

		void OnagerPhysicsSubystem2D::startup()
		{
			registerBroadphase<NSquaredBroadphase>();
			registerBroadphase<SweepAndPruneBroadphase>();
			registerBroadphase<SpatialHashBroadphase2D>();

			setBroadphase<SpatialHashBroadphase2D>();
		}

		void OnagerPhysicsSubystem2D::update_fixed()
		{
			// Update Dynamic Objects
			updateDynamics();

			const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

			// Copy component transform into collider
			for (const auto collider : mColliders)
			{
                const auto& transform = registry->get<const TransformComponent2D>(mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(collider->uuid));

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

		void OnagerPhysicsSubystem2D::endPlay()
		{
			const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

			registry->clear<BoxComponent2D>();
			registry->clear<CircleComponent2D>();
			registry->clear<RigidbodyComponent2D>();

			mCollisionPairs.clear();
			mCollisionContacts.clear();
		}

		void OnagerPhysicsSubystem2D::onConstructBox(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);
			const auto& box = registry.get<const BoxComponent2D>(entity);

            initBox(entity, id, box);
		}

		void OnagerPhysicsSubystem2D::onDestroyBox(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);

            cleanupBox(id);
		}

		void OnagerPhysicsSubystem2D::onConstructCircle(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);
			const auto& circle = registry.get<const CircleComponent2D>(entity);

            initCircle(entity, id, circle);
		}

		void OnagerPhysicsSubystem2D::onDestroyCircle(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);

            cleanupCircle(id);
		}

		void OnagerPhysicsSubystem2D::onConstructRigidbody(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);

            if (mShapes.Contains(id))
			{
                const auto collider = std::make_shared<collision2D::BoxCollider2D>(id, &mBoxShapes[id]);

                insertCollider(id, collider);
			}
		}

		void OnagerPhysicsSubystem2D::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
		{
            const auto ecs_system = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            const auto id = ecs_system->GetID(entity);

            eraseCollider(id);
		}

		//--------------------------------------------------
		// Private Functions
		//--------------------------------------------------

        void OnagerPhysicsSubystem2D::initCircle(const entt::entity& entity, const puffin::UUID& id, const CircleComponent2D& circle)
		{
			// If there is no shape for this entity in vector
            if (!mCircleShapes.Contains(id))
			{
                mCircleShapes.Emplace(id, CircleShape2D());
			}

            mCircleShapes[id].centreOfMass = circle.centreOfMass;
            mCircleShapes[id].radius = circle.radius;

			if (mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry()->all_of<RigidbodyComponent2D>(entity))
			{
				// If there is no collider for this entity, create new one
                const auto collider = std::make_shared<collision2D::CircleCollider2D>(id, &mCircleShapes[id]);

                insertCollider(id, collider);
			}
		}

        void OnagerPhysicsSubystem2D::cleanupCircle(const puffin::UUID& id)
		{
            if (mCircleShapes.Contains(id))
			{
                mCircleShapes.Erase(id);
                mShapes.Erase(id);
			}

            eraseCollider(id);
		}

        void OnagerPhysicsSubystem2D::initBox(const entt::entity& entity, const puffin::UUID& id, const BoxComponent2D& box)
		{
			// If there is no shape for this entity in vector
            if (!mBoxShapes.Contains(id))
			{
                mBoxShapes.Emplace(id, BoxShape2D());
			}

            mBoxShapes[id].centreOfMass = box.centreOfMass;
            mBoxShapes[id].half_extent = box.halfExtent;
            mBoxShapes[id].UpdatePoints();

            if (!mShapes.Contains(id))
			{
                mShapes.Emplace(id, nullptr);

                mShapes[id] = &mBoxShapes[id];

                if (mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry()->all_of<RigidbodyComponent2D>(entity) && !mColliders.Contains(id))
				{
                    const auto collider = std::make_shared<collision2D::BoxCollider2D>(id, &mBoxShapes[id]);

                    insertCollider(id, collider);
				}
			}
		}

        void OnagerPhysicsSubystem2D::cleanupBox(const puffin::UUID& id)
		{
            if (mBoxShapes.Contains(id))
                mBoxShapes.Erase(id);

            eraseCollider(id);
		}

        void OnagerPhysicsSubystem2D::insertCollider(const puffin::UUID& id, std::shared_ptr<collision2D::Collider2D> collider)
		{
			if (!mColliders.Contains(id))
			{
                mColliders.Emplace(id, collider);

				mCollidersUpdated = true;
			}
		}

        void OnagerPhysicsSubystem2D::eraseCollider(const puffin::UUID& id)
		{
			if (mColliders.Contains(id))
			{
				mColliders.Erase(id);

				mCollidersUpdated = true;
			}
		}

		void OnagerPhysicsSubystem2D::updateDynamics() const
		{
			const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

			const auto enkiTSSubSystem = mEngine->GetSubsystem<core::EnkiTSSubsystem>();

			const uint32_t numThreads = enkiTSSubSystem->GetTaskScheduler()->GetNumTaskThreads();

			const auto rbView = registry->view<RigidbodyComponent2D>();
			const auto tView = registry->view<TransformComponent2D>();
			const auto vView = registry->view<VelocityComponent2D>();

			if (rbView.empty())
			{
				return;
			}

			// Apply gravity to rigibodies as impulse

			const int numRigidbodiesPerThread = std::ceil(rbView.size() / numThreads);

			// PUFFIN_TODO - Reimplement
			//enki::TaskSet gravityTask(rbView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			//{
			//	for (uint32_t idx = range.start; idx < range.end; idx++)
			//	{
			//		const auto entity = rbView[idx];

			//		if (!registry->all_of<TransformComponent2D, VelocityComponent2D>(entity))
			//			continue;

			//		auto& rb = rbView.get<RigidbodyComponent2D>(entity);

			//		// If a body has no mass, then it is kinematic/static and should not experience forces
   //                 if (rb.mass == 0.0f || rb.bodyType != BodyType::Dynamic)
			//			continue;

			//		calculateImpulseByGravity(rb);
			//	}
			//});

			//enkiTSSubSystem->GetTaskScheduler()->AddTaskSetToPipe(&gravityTask);
			//enkiTSSubSystem->GetTaskScheduler()->WaitforTask(&gravityTask);

			// Apply velocity to transform

			std::vector<std::vector<entt::entity>> updatedEntities;
			updatedEntities.resize(numThreads);

			for (auto& entities : updatedEntities)
			{
				entities.reserve(std::max(10, numRigidbodiesPerThread));
			}

			// PUFFIN_TODO - Reimplement
			//enki::TaskSet transformTask(tView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			//{
			//	for (uint32_t idx = range.start; idx < range.end; idx++)
			//	{
			//		const auto entity = rbView[idx];

			//		if (!registry->all_of<RigidbodyComponent2D, VelocityComponent2D>(entity))
			//			continue;

			//		auto& transform = tView.get<TransformComponent2D>(entity);
			//		const auto& rb = registry->get<RigidbodyComponent2D>(entity);

			//		// If a body has no mass, then it is kinematic/static and should not experience forces
   //                 if (rb.mass == 0.0f || rb.body_type != BodyType::Dynamic)
			//			continue;

			//		// Update Position
   //                 transform.position += rb.linear_velocity * mEngine->time_step_fixed();

			//		// Update Rotation
   //                 transform.rotation += rb.angular_velocity * mEngine->time_step_fixed();

			//		updatedEntities[threadnum].emplace_back(entity);
			//	}
			//});

			//enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&transformTask);
			//enkiTSSubSystem->getTaskScheduler()->WaitforTask(&transformTask);

			for (auto& entities : updatedEntities)
			{
				for (const auto& entity : entities)
				{
					registry->patch<TransformComponent2D>(entity, [](auto& transform) {});
				}

				entities.clear();
			}

			// Update velocity component

			// PUFFIN_TODO - Reimplement
			//enki::TaskSet velocityTask(vView.size(), [&](enki::TaskSetPartition range, uint32_t threadnum)
			//{
			//	for (uint32_t idx = range.start; idx < range.end; idx++)
			//	{
			//		const auto entity = rbView[idx];

			//		if (!registry->all_of<RigidbodyComponent2D, TransformComponent2D>(entity))
			//			continue;

			//		auto& velocity = vView.get<VelocityComponent2D>(entity);
			//		const auto& rb = registry->get<RigidbodyComponent2D>(entity);

			//		// If a body has no mass, then it is kinematic/static and should not experience forces
   //                 if (rb.mass == 0.0f || rb.body_type != BodyType::Dynamic)
			//			continue;

   //                 velocity.linear.x = rb.linear_velocity.x;
   //                 velocity.linear.y = rb.linear_velocity.y;
   //                 velocity.angular = rb.angular_velocity;
			//	}
			//});

			//enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&velocityTask);
			//enkiTSSubSystem->getTaskScheduler()->WaitforTask(&velocityTask);
		}

		void OnagerPhysicsSubystem2D::calculateImpulseByGravity(RigidbodyComponent2D& body) const
		{
			// PUFFIN_TODO - Reimplement
			/*if (body.mass == 0.0f)
				return;

			const float mass = 1.0f / body.mass;
			const Vector2 impulseGravity = mGravity * mass * mEngine->GetTimeStepFixed();
			
			ApplyLinearImpulse(body, impulseGravity);*/
		}

		void OnagerPhysicsSubystem2D::collisionBroadphase()
		{
			if (mColliders.Size() == 0)
			{
				return;
			}

			mCollisionPairs.clear();
			mCollisionPairs.reserve(mColliders.size() * mColliders.size());

			// Perform Collision Broadphase to Generate Collision Pairs
			if (mActiveBroadphase)
				mActiveBroadphase->generateCollisionPairs(mColliders, mCollisionPairs, mCollidersUpdated);
		}

		void OnagerPhysicsSubystem2D::collisionDetection()
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

		void OnagerPhysicsSubystem2D::collisionResponse() const
		{
			const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

			for (const collision2D::Contact& contact : mCollisionContacts)
			{
                const auto entityA = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(contact.a);
                const auto entityB = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetEntity(contact.b);

				if (registry->all_of<RigidbodyComponent2D, VelocityComponent2D>(entityA) && registry->all_of<RigidbodyComponent2D, VelocityComponent2D>(entityB))
				{
					auto& bodyA = registry->get<RigidbodyComponent2D>(entityA);
					auto& bodyB = registry->get<RigidbodyComponent2D>(entityB);

					auto& velocityA = registry->get<VelocityComponent2D>(entityA);
					auto& velocityB = registry->get<VelocityComponent2D>(entityB);

					const float elasticity = bodyA.elasticity * bodyB.elasticity;

					// Calculate Collision Impulse
                    const Vector2 vab = velocityB.linear - velocityB.linear;
					const float nVab = vab.Dot(contact.normal);

					/*if (nVab <= 0)
						continue;*/

					const float impulseJ = -(1.0f + elasticity) * nVab / (bodyA.mass + bodyB.mass);
					const Vector2 vectorImpulseJ = contact.normal * impulseJ;

					// Apply Impulse to body A
					ApplyImpulse(bodyA, velocityA, contact.pointOnA, vectorImpulseJ * 1.0f);

					// Apply Impulse to body B
					ApplyImpulse(bodyB, velocityB, contact.pointOnB, vectorImpulseJ * -1.0f);

					// Calculate impulse caused by friction


					// Move colliding bodies to just outside each other
					const float tA = bodyA.mass / (bodyA.mass + bodyB.mass);
					const float tB = bodyB.mass / (bodyA.mass + bodyB.mass);

					const Vector2 ds = (contact.pointOnB - contact.pointOnA) * contact.normal.Abs();

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

		void OnagerPhysicsSubystem2D::generateCollisionEvents()
		{
			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

			std::set<collision2D::Contact> existingContacts;

			// Iterate over contacts for this tick
			for (const collision2D::Contact& contact : mCollisionContacts)
			{
				// Publish Begin Collision Event when a contact was not in active contacts set
				if (mActiveContacts.count(contact) == 0)
				{
					mActiveContacts.insert(contact);

					signalSubsystem->Emit<CollisionBeginEvent>("CollisionBegin", { contact.a, contact.b });
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

				signalSubsystem->Emit<CollisionEndEvent>("CollisionEnd", { contact.a, contact.b });
			}
		}
	}
}

#endif // PFN_ONAGER2D_PHYSICS

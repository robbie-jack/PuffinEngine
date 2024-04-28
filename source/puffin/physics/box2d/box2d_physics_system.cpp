#include "puffin/physics/box2d/box2d_physics_system.h"

#if PFN_BOX2D_PHYSICS

#include "MathHelpers.h"
#include "puffin/core/engine.h"
#include "puffin/core/SignalSubsystem.h"

#include "Components/SceneObjectComponent.h"
#include "Components/TransformComponent2D.h"
#include "Components/Physics/2D/RigidbodyComponent2D.h"
#include "Components/Physics/2D/ShapeComponents2D.h"
#include "Components/Physics/2D/VelocityComponent2D.h"

namespace puffin::physics
{
	void Box2DPhysicsSystem::beginPlay()
	{
		// Create Physics World
		mPhysicsWorld = std::make_unique<b2World>(mGravity);

		//// Create Contact Listener and pass it to physics world
		mContactListener = std::make_unique<Box2DContactListener>();
		mPhysicsWorld->SetContactListener(mContactListener.get());

		updateComponents();
	}

	void Box2DPhysicsSystem::fixedUpdate()
	{
		updateComponents();

		// Step Physics World
		mPhysicsWorld->Step(mEngine->timeStepFixed(), mVelocityIterations, mPositionIterations);

		// Publish Collision Events
		publishCollisionEvents();

		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		// Updated entity position/rotation from simulation
		const auto bodyView = registry->view<const SceneObjectComponent, TransformComponent2D, VelocityComponent2D, const RigidbodyComponent2D>();

		for (auto [entity, object, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = object.id;

			// Update Transform from Rigidbody Position
			registry->patch<TransformComponent2D>(entity, [&](auto& transform)
			{
				transform.position.x = mBodies[id]->GetPosition().x;
				transform.position.y = mBodies[id]->GetPosition().y;
				transform.rotation = maths::radToDeg(-mBodies[id]->GetAngle());
			});

			// Update Velocity with Linear/Angular Velocity#
			registry->patch<VelocityComponent2D>(entity, [&](auto& velocity)
			{
				velocity.linear.x = mBodies[id]->GetLinearVelocity().x;
				velocity.linear.y = mBodies[id]->GetLinearVelocity().y;
			});
		}
	}

	void Box2DPhysicsSystem::endPlay()
	{
		mCircleShapes.clear();
		mPolygonShapes.clear();
		mShapes.clear();
		mBodies.clear();
		mFixtures.clear();

		mPhysicsWorld = nullptr;
		mContactListener = nullptr;
	}

	void Box2DPhysicsSystem::onConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);
		
		mBoxesToInit.push_back(object.id);
	}

	void Box2DPhysicsSystem::onDestroyBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupBox(object.id);
		cleanupFixture(object.id);
	}

	void Box2DPhysicsSystem::onConstructCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mCirclesToInit.push_back(object.id);
	}

	void Box2DPhysicsSystem::onDestroyCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupCircle(object.id);
		cleanupFixture(object.id);
	}

	void Box2DPhysicsSystem::onConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mRigidbodiesToInit.push_back(object.id);
	}

	void Box2DPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupRigidbody(object.id);
		cleanupFixture(object.id);
	}

	void Box2DPhysicsSystem::updateComponents()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		// Update Circles
		{
			for (const auto& id : mCirclesToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent2D>(entity);
				const auto& circle = registry->get<const CircleComponent2D>(entity);

				initCircle(object.id, transform, circle);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					initFixture(object.id, rb);
				}
			}

			mCirclesToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : mBoxesToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent2D>(entity);
				const auto& box = registry->get<const BoxComponent2D>(entity);

				initBox(object.id, transform, box);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					initFixture(object.id, rb);
				}
			}

			mBoxesToInit.clear();
		}

		// Update Rigidbodies
		{
			for (const auto& id : mRigidbodiesToInit)
			{
				entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->getEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent2D>(entity);
				const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

				initRigidbody(object.id, transform, rb);
				initFixture(object.id, rb);
			}

			mRigidbodiesToInit.clear();
		}
	}

	void Box2DPhysicsSystem::publishCollisionEvents() const
	{
		const auto signalSubsystem = mEngine->getSystem<core::SignalSubsystem>();

		CollisionBeginEvent collisionBeginEvent;
		while (mContactListener->getNextCollisionBeginEvent(collisionBeginEvent))
		{
			//signalSubsystem->signal(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (mContactListener->getNextCollisionEndEvent(collisionEndEvent))
		{
			//signalSubsystem->signal(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::initRigidbody(PuffinID id, const TransformComponent2D& transform, const RigidbodyComponent2D& rb)
	{
		if (!mBodies.contains(id))
		{
			b2BodyDef bodyDef;
			bodyDef.userData.pointer = id;
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = -maths::degToRad(transform.rotation);
			bodyDef.type = gBodyType.at(rb.bodyType);

			// Created Body from Physics World
			mBodies.emplace(id, mPhysicsWorld->CreateBody(&bodyDef));

			b2MassData massData = {};
			massData.mass = rb.mass;

			mBodies[id]->SetMassData(&massData);
		}
	}

	void Box2DPhysicsSystem::initBox(PuffinID id, const TransformComponent2D& transform, const BoxComponent2D& box)
	{
		if (!mPolygonShapes.contains(id))
		{
			mPolygonShapes.insert(id, b2PolygonShape());
		}

		mPolygonShapes[id].SetAsBox(box.halfExtent.x, box.halfExtent.y, box.centreOfMass, 0.0f);

		if (!mShapes.contains(id))
		{
			mShapes.insert(id, &mPolygonShapes[id]);
		}
	}

	void Box2DPhysicsSystem::initCircle(PuffinID id, const TransformComponent2D& transform, const CircleComponent2D& circle)
	{
		if (!mCircleShapes.contains(id))
		{
			mCircleShapes.insert(id, b2CircleShape());
		}

		mCircleShapes[id].m_radius = circle.radius;
		mCircleShapes[id].m_p.Set(transform.position.x, transform.position.y);

		if (!mShapes.contains(id))
		{
			mShapes.insert(id, &mCircleShapes[id]);
		}
	}

	void Box2DPhysicsSystem::initFixture(PuffinID id, const RigidbodyComponent2D rb)
	{
		if (mBodies.contains(id) && mShapes.contains(id) && !mFixtures.contains(id))
		{
			b2FixtureDef fixtureDef;
			fixtureDef.shape = mShapes[id];
			fixtureDef.restitution = rb.elasticity;
			fixtureDef.density = 1.0f;
			fixtureDef.friction = 0.3f;

			mFixtures.emplace(id, mBodies[id]->CreateFixture(&fixtureDef));
		}
	}

	void Box2DPhysicsSystem::updateRigidbody(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::updateBox(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::updateCircle(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::cleanupRigidbody(PuffinID id)
	{
		if (mBodies.contains(id))
		{
			mPhysicsWorld->DestroyBody(mBodies[id]);
			mBodies[id] = nullptr;
			mBodies.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupBox(PuffinID id)
	{
		if (mPolygonShapes.contains(id))
		{
			mPolygonShapes.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupCircle(PuffinID id)
	{
		if (mCircleShapes.contains(id))
		{
			mCircleShapes.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupFixture(PuffinID id)
	{
		if (mFixtures.contains(id))
		{
			mBodies[id]->DestroyFixture(mFixtures[id]);
			mFixtures[id] = nullptr;
			mFixtures.erase(id);
		}
	}
}

#endif // PFN_BOX2D_PHYSICS
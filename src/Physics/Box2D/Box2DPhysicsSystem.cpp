#include "Physics/Box2D/Box2DPhysicsSystem.h"

#include "Engine\Engine.h"
#include "Engine\SignalSubsystem.h"

#include "Components/SceneObjectComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.h"

namespace puffin::physics
{
	void Box2DPhysicsSystem::start()
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

		const auto registry = mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Updated entity position/rotation from simulation
		const auto bodyView = registry->view<const SceneObjectComponent, TransformComponent, VelocityComponent, const RigidbodyComponent2D>();

		for (auto [entity, object, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = object.uuid;

			// Update Transform from Rigidbody Position
			transform.position.x = mBodies[id]->GetPosition().x;
			transform.position.y = mBodies[id]->GetPosition().y;
			transform.rotation = maths::Quat::fromEulerAngles(0.0, 0.0, -mBodies[id]->GetAngle());

			// Update Velocity with Linear/Angular Velocity
			velocity.linear.x = mBodies[id]->GetLinearVelocity().x;
			velocity.linear.y = mBodies[id]->GetLinearVelocity().y;
			velocity.angular.z = mBodies[id]->GetAngularVelocity();
		}
	}

	void Box2DPhysicsSystem::stop()
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
		
		mBoxesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::onDestroyBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupBox(object.uuid);
		cleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::onConstructCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mCirclesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::onDestroyCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupCircle(object.uuid);
		cleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::onConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		mRigidbodiesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupRigidbody(object.uuid);
		cleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::updateComponents()
	{
		const auto registry = mEngine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Update Circles
		{
			for (const auto& id : mCirclesToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& circle = registry->get<const CircleComponent2D>(entity);

				initCircle(object.uuid, transform, circle);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					initFixture(object.uuid, rb);
				}
			}

			mCirclesToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : mBoxesToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& box = registry->get<const BoxComponent2D>(entity);

				initBox(object.uuid, transform, box);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					initFixture(object.uuid, rb);
				}
			}

			mBoxesToInit.clear();
		}

		// Update Rigidbodies
		{
			for (const auto& id : mRigidbodiesToInit)
			{
				entt::entity entity = mEngine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

				initRigidbody(object.uuid, transform, rb);
				initFixture(object.uuid, rb);
			}

			mRigidbodiesToInit.clear();
		}
	}

	void Box2DPhysicsSystem::publishCollisionEvents() const
	{
		const auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

		CollisionBeginEvent collisionBeginEvent;
		while (mContactListener->getNextCollisionBeginEvent(collisionBeginEvent))
		{
			signalSubsystem->signal(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (mContactListener->getNextCollisionEndEvent(collisionEndEvent))
		{
			signalSubsystem->signal(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::initRigidbody(PuffinId id, const TransformComponent& transform, const RigidbodyComponent2D& rb)
	{
		if (!mBodies.contains(id))
		{
			b2BodyDef bodyDef;
			bodyDef.userData.pointer = static_cast<uintptr_t>(id);
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = -transform.rotation.eulerAnglesRad().z;
			bodyDef.type = gBodyType.at(rb.bodyType);

			// Created Body from Physics World
			mBodies.emplace(id, mPhysicsWorld->CreateBody(&bodyDef));

			b2MassData massData = {};
			massData.mass = rb.mass;

			mBodies[id]->SetMassData(&massData);
		}
	}

	void Box2DPhysicsSystem::initBox(PuffinId id, const TransformComponent& transform, const BoxComponent2D& box)
	{
		if (!mPolygonShapes.contains(id))
		{
			mPolygonShapes.insert(id, b2PolygonShape());
		}

		mPolygonShapes[id].SetAsBox(box.halfExtent.x, box.halfExtent.y, transform.position.GetXY(), transform.rotation.eulerAnglesRad().z);

		if (!mShapes.contains(id))
		{
			mShapes.insert(id, &mPolygonShapes[id]);
		}
	}

	void Box2DPhysicsSystem::initCircle(PuffinId id, const TransformComponent& transform, const CircleComponent2D& circle)
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

	void Box2DPhysicsSystem::initFixture(PuffinId id, const RigidbodyComponent2D rb)
	{
		if (mBodies.contains(id) && mShapes.contains(id) && !mFixtures.contains(id))
		{
			b2FixtureDef fixtureDef;
			fixtureDef.shape = mShapes[id];
			fixtureDef.restitution = rb.elasticity;

			mFixtures.emplace(id, mBodies[id]->CreateFixture(&fixtureDef));
		}
	}

	void Box2DPhysicsSystem::updateRigidbody(PuffinId id)
	{

	}

	void Box2DPhysicsSystem::updateBox(PuffinId id)
	{

	}

	void Box2DPhysicsSystem::updateCircle(PuffinId id)
	{

	}

	void Box2DPhysicsSystem::cleanupRigidbody(PuffinId id)
	{
		if (mBodies.contains(id))
		{
			mPhysicsWorld->DestroyBody(mBodies[id]);
			mBodies[id] = nullptr;
			mBodies.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupBox(PuffinId id)
	{
		if (mPolygonShapes.contains(id))
		{
			mPolygonShapes.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupCircle(PuffinId id)
	{
		if (mCircleShapes.contains(id))
		{
			mCircleShapes.erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupFixture(PuffinId id)
	{
		if (mFixtures.contains(id))
		{
			mBodies[id]->DestroyFixture(mFixtures[id]);
			mFixtures[id] = nullptr;
			mFixtures.erase(id);
		}
	}
}

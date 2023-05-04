#include "Physics/Box2D/Box2DPhysicsSystem.h"

#include "Engine/Engine.hpp"
#include "Engine/SignalSubsystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.hpp"

namespace puffin::physics
{
	void Box2DPhysicsSystem::init()
	{
		auto eventSubsystem = m_engine->getSubsystem<Core::EventSubsystem>();

		// Register Events
		eventSubsystem->RegisterEvent<CollisionBeginEvent>();
		eventSubsystem->RegisterEvent<CollisionEndEvent>();
	}

	void Box2DPhysicsSystem::start()
	{
		// Create Physics World
		physicsWorld_ = std::make_unique<b2World>(gravity_);

		//// Create Contact Listener and pass it to physics world
		contactListener_ = std::make_unique<Box2DContactListener>();
		physicsWorld_->SetContactListener(contactListener_.get());

		updateComponents();
	}

	void Box2DPhysicsSystem::fixedUpdate()
	{
		updateComponents();

		// Step Physics World
		physicsWorld_->Step(m_engine->timeStepFixed(), velocityIterations_, positionIterations_);

		// Publish Collision Events
		publishCollisionEvents();

		auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Updated entity position/rotation from simulation
		auto bodyView = registry->view<const SceneObjectComponent, TransformComponent, VelocityComponent, const RigidbodyComponent2D>();

		for (auto [entity, object, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = object.uuid;

			// Update Transform from Rigidbody Position
			transform.position.x = bodies_[id]->GetPosition().x;
			transform.position.y = bodies_[id]->GetPosition().y;
			transform.rotation = Maths::Quat::FromEulerAngles(0.0, 0.0, -bodies_[id]->GetAngle());

			// Update Velocity with Linear/Angular Velocity
			velocity.linear.x = bodies_[id]->GetLinearVelocity().x;
			velocity.linear.y = bodies_[id]->GetLinearVelocity().y;
			velocity.angular.z = bodies_[id]->GetAngularVelocity();
		}
	}

	void Box2DPhysicsSystem::stop()
	{
		circleShapes_.Clear();
		polygonShapes_.Clear();
		shapes_.Clear();
		bodies_.Clear();
		fixtures_.Clear();

		physicsWorld_ = nullptr;
		contactListener_ = nullptr;
	}

	void Box2DPhysicsSystem::onConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);
		
		boxesToInit_.push_back(object.uuid);
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

		circlesToInit_.push_back(object.uuid);
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

		rigidbodiesToInit_.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::onDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		cleanupRigidbody(object.uuid);
		cleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::updateComponents()
	{
		auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Update Circles
		{
			for (const auto& id : circlesToInit_)
			{
				entt::entity entity = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

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

			circlesToInit_.clear();
		}

		// Update Boxes
		{
			for (const auto& id : boxesToInit_)
			{
				entt::entity entity = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

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

			boxesToInit_.clear();
		}

		// Update Rigidbodies
		{
			for (const auto& id : rigidbodiesToInit_)
			{
				entt::entity entity = m_engine->getSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

				initRigidbody(object.uuid, transform, rb);
				initFixture(object.uuid, rb);
			}

			rigidbodiesToInit_.clear();
		}
	}

	void Box2DPhysicsSystem::publishCollisionEvents() const
	{
		const auto eventSubsystem = m_engine->getSubsystem<Core::EventSubsystem>();
		const auto signalSubsystem = m_engine->getSubsystem<Core::SignalSubsystem>();

		CollisionBeginEvent collisionBeginEvent;
		while (contactListener_->getNextCollisionBeginEvent(collisionBeginEvent))
		{
			eventSubsystem->Publish(collisionBeginEvent);
			signalSubsystem->Signal(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (contactListener_->getNextCollisionEndEvent(collisionEndEvent))
		{
			eventSubsystem->Publish(collisionEndEvent);
			signalSubsystem->Signal(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::initRigidbody(UUID id, const TransformComponent& transform, const RigidbodyComponent2D& rb)
	{
		if (!bodies_.Contains(id))
		{
			b2BodyDef bodyDef;
			bodyDef.userData.pointer = static_cast<uintptr_t>(id);
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = -transform.rotation.EulerAnglesRad().z;
			bodyDef.type = gBodyType.at(rb.bodyType);

			// Created Body from Physics World
			bodies_.Emplace(id, physicsWorld_->CreateBody(&bodyDef));

			b2MassData massData = {};
			massData.mass = rb.mass;

			bodies_[id]->SetMassData(&massData);
		}
	}

	void Box2DPhysicsSystem::initBox(UUID id, const TransformComponent& transform, const BoxComponent2D& box)
	{
		if (!polygonShapes_.Contains(id))
		{
			polygonShapes_.Insert(id, b2PolygonShape());
		}

		polygonShapes_[id].SetAsBox(box.halfExtent.x, box.halfExtent.y, transform.position.GetXY(), transform.rotation.EulerAnglesRad().z);

		if (!shapes_.Contains(id))
		{
			shapes_.Insert(id, &polygonShapes_[id]);
		}
	}

	void Box2DPhysicsSystem::initCircle(UUID id, const TransformComponent& transform, const CircleComponent2D& circle)
	{
		if (!circleShapes_.Contains(id))
		{
			circleShapes_.Insert(id, b2CircleShape());
		}

		circleShapes_[id].m_radius = circle.radius;
		circleShapes_[id].m_p.Set(transform.position.x, transform.position.y);

		if (!shapes_.Contains(id))
		{
			shapes_.Insert(id, &circleShapes_[id]);
		}
	}

	void Box2DPhysicsSystem::initFixture(UUID id, const RigidbodyComponent2D rb)
	{
		if (bodies_.Contains(id) && shapes_.Contains(id) && !fixtures_.Contains(id))
		{
			b2FixtureDef fixtureDef;
			fixtureDef.shape = shapes_[id];
			fixtureDef.restitution = rb.elasticity;

			fixtures_.Emplace(id, bodies_[id]->CreateFixture(&fixtureDef));
		}
	}

	void Box2DPhysicsSystem::updateRigidbody(UUID id)
	{

	}

	void Box2DPhysicsSystem::updateBox(UUID id)
	{

	}

	void Box2DPhysicsSystem::updateCircle(UUID id)
	{

	}

	void Box2DPhysicsSystem::cleanupRigidbody(UUID id)
	{
		if (bodies_.Contains(id))
		{
			physicsWorld_->DestroyBody(bodies_[id]);
			bodies_[id] = nullptr;
			bodies_.Erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupBox(UUID id)
	{
		if (polygonShapes_.Contains(id))
		{
			polygonShapes_.Erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupCircle(UUID id)
	{
		if (circleShapes_.Contains(id))
		{
			circleShapes_.Erase(id);
		}
	}

	void Box2DPhysicsSystem::cleanupFixture(UUID id)
	{
		if (fixtures_.Contains(id))
		{
			bodies_[id]->DestroyFixture(fixtures_[id]);
			fixtures_[id] = nullptr;
			fixtures_.Erase(id);
		}
	}
}

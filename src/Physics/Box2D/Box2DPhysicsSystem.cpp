#include "Physics/Box2D/Box2DPhysicsSystem.h"

#include "Engine/Engine.hpp"
#include "Engine/SignalSubsystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.hpp"

namespace Puffin::Physics
{
	void Box2DPhysicsSystem::Init()
	{
		auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

		// Register Events
		eventSubsystem->RegisterEvent<CollisionBeginEvent>();
		eventSubsystem->RegisterEvent<CollisionEndEvent>();
	}

	void Box2DPhysicsSystem::Start()
	{
		// Create Physics World
		m_physicsWorld = std::make_unique<b2World>(m_gravity);

		//// Create Contact Listener and pass it to physics world
		m_contactListener = std::make_unique<Box2DContactListener>();
		m_physicsWorld->SetContactListener(m_contactListener.get());

		UpdateComponents();
	}

	void Box2DPhysicsSystem::FixedUpdate()
	{
		UpdateComponents();

		// Step Physics World
		m_physicsWorld->Step(m_engine->GetTimeStep(), m_velocityIterations, m_positionIterations);

		// Publish Collision Events
		PublishCollisionEvents();

		auto registry = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Updated entity position/rotation from simulation
		auto bodyView = registry->view<const SceneObjectComponent, TransformComponent, VelocityComponent, const RigidbodyComponent2D>();

		for (auto [entity, object, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = object.uuid;

			// Update Transform from Rigidbody Position
			transform.position.x = m_bodies[id]->GetPosition().x;
			transform.position.y = m_bodies[id]->GetPosition().y;
			transform.rotation = Maths::Quat::FromEulerAngles(0.0, 0.0, -m_bodies[id]->GetAngle());

			// Update Velocity with Linear/Angular Velocity
			velocity.linear.x = m_bodies[id]->GetLinearVelocity().x;
			velocity.linear.y = m_bodies[id]->GetLinearVelocity().y;
			velocity.angular.z = m_bodies[id]->GetAngularVelocity();
		}
	}

	void Box2DPhysicsSystem::Stop()
	{
		m_circleShapes.Clear();
		m_polygonShapes.Clear();
		m_shapes.Clear();
		m_bodies.Clear();
		m_fixtures.Clear();

		m_physicsWorld = nullptr;
		m_contactListener = nullptr;
	}

	void Box2DPhysicsSystem::OnConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);
		
		m_boxesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		CleanupBoxComponent(object.uuid);
		CleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::OnConstructCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		m_circlesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::OnDestroyCircle(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		CleanupCircleComponent(object.uuid);
		CleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::OnConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		m_rigidbodiesToInit.push_back(object.uuid);
	}

	void Box2DPhysicsSystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto& object = registry.get<const SceneObjectComponent>(entity);

		CleanupRigidbodyComponent(object.uuid);
		CleanupFixture(object.uuid);
	}

	void Box2DPhysicsSystem::UpdateComponents()
	{
		auto registry = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->Registry();

		// Update Circles
		{
			for (const auto& id : m_circlesToInit)
			{
				entt::entity entity = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& circle = registry->get<const CircleComponent2D>(entity);

				InitCircleComponent(object.uuid, transform, circle);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					InitFixture(object.uuid, rb);
				}
			}

			m_circlesToInit.clear();
		}

		// Update Boxes
		{
			for (const auto& id : m_boxesToInit)
			{
				entt::entity entity = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& box = registry->get<const BoxComponent2D>(entity);

				InitBoxComponent(object.uuid, transform, box);

				if (registry->all_of<RigidbodyComponent2D>(entity))
				{
					const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

					InitFixture(object.uuid, rb);
				}
			}

			m_boxesToInit.clear();
		}

		// Update Rigidbodies
		{
			for (const auto& id : m_rigidbodiesToInit)
			{
				entt::entity entity = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->GetEntity(id);

				const auto& object = registry->get<const SceneObjectComponent>(entity);
				const auto& transform = registry->get<const TransformComponent>(entity);
				const auto& rb = registry->get<const RigidbodyComponent2D>(entity);

				InitRigidbodyComponent(object.uuid, transform, rb);
				InitFixture(object.uuid, rb);
			}

			m_rigidbodiesToInit.clear();
		}
	}

	void Box2DPhysicsSystem::PublishCollisionEvents() const
	{
		auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();
		auto signalSubsystem = m_engine->GetSubsystem<Core::SignalSubsystem>();

		CollisionBeginEvent collisionBeginEvent;
		while (m_contactListener->GetNextCollisionBeginEvent(collisionBeginEvent))
		{
			eventSubsystem->Publish(collisionBeginEvent);
			signalSubsystem->Signal(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (m_contactListener->GetNextCollisionEndEvent(collisionEndEvent))
		{
			eventSubsystem->Publish(collisionEndEvent);
			signalSubsystem->Signal(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::InitRigidbodyComponent(UUID id, const TransformComponent& transform, const RigidbodyComponent2D& rb)
	{
		if (!m_bodies.Contains(id))
		{
			b2BodyDef bodyDef;
			bodyDef.userData.pointer = static_cast<uintptr_t>(id);
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = -transform.rotation.EulerAnglesRad().z;
			bodyDef.type = G_BODY_TYPE_MAP.at(rb.bodyType);

			// Created Body from Physics World
			m_bodies.Emplace(id, m_physicsWorld->CreateBody(&bodyDef));

			b2MassData massData = {};
			massData.mass = rb.mass;

			m_bodies[id]->SetMassData(&massData);
		}
	}

	void Box2DPhysicsSystem::InitBoxComponent(UUID id, const TransformComponent& transform, const BoxComponent2D& box)
	{
		if (!m_polygonShapes.Contains(id))
		{
			m_polygonShapes.Insert(id, b2PolygonShape());
		}

		m_polygonShapes[id].SetAsBox(box.halfExtent.x, box.halfExtent.y, transform.position.GetXY(), transform.rotation.EulerAnglesRad().z);

		if (!m_shapes.Contains(id))
		{
			m_shapes.Insert(id, &m_polygonShapes[id]);
		}
	}

	void Box2DPhysicsSystem::InitCircleComponent(UUID id, const TransformComponent& transform, const CircleComponent2D& circle)
	{
		if (!m_circleShapes.Contains(id))
		{
			m_circleShapes.Insert(id, b2CircleShape());
		}

		m_circleShapes[id].m_radius = circle.radius;
		m_circleShapes[id].m_p.Set(transform.position.x, transform.position.y);

		if (!m_shapes.Contains(id))
		{
			m_shapes.Insert(id, &m_circleShapes[id]);
		}
	}

	void Box2DPhysicsSystem::InitFixture(UUID id, const RigidbodyComponent2D rb)
	{
		if (m_bodies.Contains(id) && m_shapes.Contains(id) && !m_fixtures.Contains(id))
		{
			b2FixtureDef fixtureDef;
			fixtureDef.shape = m_shapes[id];
			fixtureDef.restitution = rb.elasticity;

			m_fixtures.Emplace(id, m_bodies[id]->CreateFixture(&fixtureDef));
		}
	}

	void Box2DPhysicsSystem::UpdateRigidbody(UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateBox(UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateCircle(UUID id)
	{

	}

	void Box2DPhysicsSystem::CleanupRigidbodyComponent(UUID id)
	{
		if (m_bodies.Contains(id))
		{
			m_physicsWorld->DestroyBody(m_bodies[id]);
			m_bodies[id] = nullptr;
			m_bodies.Erase(id);
		}
	}

	void Box2DPhysicsSystem::CleanupBoxComponent(UUID id)
	{
		if (m_polygonShapes.Contains(id))
		{
			m_polygonShapes.Erase(id);
		}
	}

	void Box2DPhysicsSystem::CleanupCircleComponent(UUID id)
	{
		if (m_circleShapes.Contains(id))
		{
			m_circleShapes.Erase(id);
		}
	}

	void Box2DPhysicsSystem::CleanupFixture(UUID id)
	{
		if (m_fixtures.Contains(id))
		{
			m_bodies[id]->DestroyFixture(m_fixtures[id]);
			m_fixtures[id] = nullptr;
			m_fixtures.Erase(id);
		}
	}
}

#include "Physics/Box2D/Box2DPhysicsSystem.h"

#include "Engine/Engine.hpp"
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "MathHelpers.h"

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

	void Box2DPhysicsSystem::PreStart()
	{
		// Create Physics World
		m_physicsWorld = std::make_unique<b2World>(m_gravity);

		// Create Contact Listener and pass it to physics world
		m_contactListener = std::make_unique<Box2DContactListener>();
		m_physicsWorld->SetContactListener(m_contactListener.get());

		// Perform Initial Setup of Components
		UpdateComponents();
	}

	void Box2DPhysicsSystem::Update()
	{
		// Initialize/Cleanup Components
		UpdateComponents();

		// Step Physics World
		m_physicsWorld->Step(m_engine->GetTimeStep(), m_velocityIterations, m_positionIterations);

		// Publish Collision Events
		PublishCollisionEvents();

		// Updated entity position/rotation from simulation
		std::vector<std::shared_ptr<ECS::Entity>> rbEntities;
		ECS::GetEntities<TransformComponent, VelocityComponent, RigidbodyComponent2D>(m_world, rbEntities);
		for (const auto& entity : rbEntities)
		{
			auto& transform = entity->GetComponent<TransformComponent>();
			auto& velocity = entity->GetComponent<VelocityComponent>();

			// Update Transform from Rigidbody Position
			transform.position.x = m_bodies[entity->ID()]->GetPosition().x;
			transform.position.y = m_bodies[entity->ID()]->GetPosition().y;
			transform.rotation = Maths::Quat::FromEulerAngles(0.0, 0.0, -m_bodies[entity->ID()]->GetAngle());

			// Update Interpolated Transform with Linear/Angular Velocity
			velocity.linear.x = m_bodies[entity->ID()]->GetLinearVelocity().x;
			velocity.linear.y = m_bodies[entity->ID()]->GetLinearVelocity().y;
			velocity.angular.z = m_bodies[entity->ID()]->GetAngularVelocity();
		}
	}

	void Box2DPhysicsSystem::Stop()
	{
		// Cleanup Rigidbody Components
		std::vector<std::shared_ptr<ECS::Entity>> rbEntities;
		ECS::GetEntities<TransformComponent, VelocityComponent, RigidbodyComponent2D>(m_world, rbEntities);
		for (const auto& entity : rbEntities)
		{
			auto& velocity = entity->GetComponent<VelocityComponent>();
			velocity.linear = Vector3f(0.0f);
			velocity.angular = Vector3f(0.0f);

			CleanupRigidbodyComponent(entity->ID());
		}

		// Cleanup Box Components
		std::vector<std::shared_ptr<ECS::Entity>> boxEntities;
		ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntities);
		for (const auto& entity : boxEntities)
		{
			CleanupBoxComponent(entity->ID());
		}

		// Cleanup Box Components
		std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
		ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
		for (const auto& entity : circleEntities)
		{
			CleanupCircleComponent(entity->ID());
		}

		m_circleShapes.Clear();
		m_polygonShapes.Clear();

		m_physicsWorld = nullptr;
		m_contactListener = nullptr;
	}

	void Box2DPhysicsSystem::PublishCollisionEvents() const
	{
		auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

		CollisionBeginEvent collisionBeginEvent;
		while (m_contactListener->GetNextCollisionBeginEvent(collisionBeginEvent))
		{
			eventSubsystem->Publish(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (m_contactListener->GetNextCollisionEndEvent(collisionEndEvent))
		{
			eventSubsystem->Publish(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::UpdateComponents()
	{
		// Update Rigidbody
		std::vector<std::shared_ptr<ECS::Entity>> rigidbodyEntities;
		ECS::GetEntities<TransformComponent, RigidbodyComponent2D>(m_world, rigidbodyEntities);
		for (const auto& entity : rigidbodyEntities)
		{
			if (entity->GetComponentFlag<RigidbodyComponent2D, FlagDirty>())
			{
				CleanupRigidbodyComponent(entity->ID());
				InitRigidbodyComponent(entity->ID());

				entity->SetComponentFlag<RigidbodyComponent2D, FlagDirty>(false);
			}

			if (entity->GetComponentFlag<RigidbodyComponent2D, FlagDeleted>())
			{
				CleanupRigidbodyComponent(entity->ID());

				entity->RemoveComponent<RigidbodyComponent2D>();
			}
		}
				
		// Update Box Components
		std::vector<std::shared_ptr<ECS::Entity>> boxEntities;
		ECS::GetEntities<TransformComponent, BoxComponent2D>(m_world, boxEntities);
		for (const auto& entity : boxEntities)
		{
			auto& box = m_world->GetComponent<BoxComponent2D>(entity->ID());

			if (entity->GetComponentFlag<BoxComponent2D, FlagDirty>())
			{
				InitBoxComponent(entity->ID());

				entity->SetComponentFlag<BoxComponent2D, FlagDirty>(false);
			}

			if (entity->GetComponentFlag<BoxComponent2D, FlagDeleted>())
			{
				CleanupBoxComponent(entity->ID());

				entity->RemoveComponent<BoxComponent2D>();
			}
		}

		// Update Circle Components
		std::vector<std::shared_ptr<ECS::Entity>> circleEntities;
		ECS::GetEntities<TransformComponent, CircleComponent2D>(m_world, circleEntities);
		for (const auto& entity : circleEntities)
		{
			auto& circle = entity->GetComponent<CircleComponent2D>();

			if (entity->GetComponentFlag<CircleComponent2D, FlagDirty>())
			{
				InitCircleComponent(entity->ID());

				entity->SetComponentFlag<CircleComponent2D, FlagDirty>(false);
			}

			if (entity->GetComponentFlag<CircleComponent2D, FlagDeleted>())
			{
				CleanupCircleComponent(entity->ID());

				entity->RemoveComponent<CircleComponent2D>();
			}
		}
	}

	void Box2DPhysicsSystem::InitRigidbodyComponent(ECS::EntityID entity)
	{
		const auto transform = m_world->GetComponent<TransformComponent>(entity);

		if (!m_bodies.Contains(entity))
		{
			m_bodies.Insert(entity, nullptr);

			b2BodyDef bodyDef;
			bodyDef.userData.pointer = static_cast<uintptr_t>(entity);
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = -transform.rotation.EulerAnglesRad().z;

			// Created Body from Physics World
			m_bodies[entity] = m_physicsWorld->CreateBody(&bodyDef);
		}
	}

	void Box2DPhysicsSystem::InitBoxComponent(ECS::EntityID entity)
	{
		auto& box = m_world->GetComponent<BoxComponent2D>(entity);
		auto& transform = m_world->GetComponent<TransformComponent>(entity);

		if (!m_polygonShapes.Contains(entity))
		{
			m_polygonShapes.Insert(entity, b2PolygonShape());
		}

		m_polygonShapes[entity].SetAsBox(box.halfExtent.x, box.halfExtent.y, transform.position.GetXY(), transform.rotation.EulerAnglesRad().z);

		if (m_world->HasComponent<RigidbodyComponent2D>(entity))
		{
			InitFixture(entity, &m_polygonShapes[entity]);
		}
	}

	void Box2DPhysicsSystem::InitCircleComponent(ECS::EntityID entity)
	{
		auto& circle = m_world->GetComponent<CircleComponent2D>(entity);
		auto& transform = m_world->GetComponent<TransformComponent>(entity);

		if (!m_circleShapes.Contains(entity))
		{
			m_circleShapes.Insert(entity, b2CircleShape());
		}

		m_circleShapes[entity].m_radius = circle.radius;
		m_circleShapes[entity].m_p.Set(transform.position.x, transform.position.y);

		if (m_world->HasComponent<RigidbodyComponent2D>(entity))
		{
			InitFixture(entity, &m_circleShapes[entity]);
		}
	}

	void Box2DPhysicsSystem::InitFixture(ECS::EntityID entity, b2Shape* shape)
	{
		if (m_bodies.Contains(entity))
		{
			b2FixtureDef fixtureDef;
			fixtureDef.shape = shape;

			if (!m_fixtures.Contains(entity))
			{
				m_fixtures.Insert(entity, nullptr);
			}

			m_fixtures[entity] = m_bodies[entity]->CreateFixture(&fixtureDef);
		}
	}

	void Box2DPhysicsSystem::CleanupRigidbodyComponent(ECS::EntityID entity)
	{
		CleanupFixture(entity);

		if (m_bodies.Contains(entity))
		{
			m_physicsWorld->DestroyBody(m_bodies[entity]);
			m_bodies[entity] = nullptr;
			m_bodies.Erase(entity);
		}
	}

	void Box2DPhysicsSystem::CleanupBoxComponent(ECS::EntityID entity)
	{
		CleanupFixture(entity);

		if (m_polygonShapes.Contains(entity))
		{
			m_polygonShapes.Erase(entity);
		}
	}

	void Box2DPhysicsSystem::CleanupCircleComponent(ECS::EntityID entity)
	{
		CleanupFixture(entity);

		if (m_circleShapes.Contains(entity))
		{
			m_circleShapes.Erase(entity);
		}
	}

	void Box2DPhysicsSystem::CleanupFixture(ECS::EntityID entity)
	{
		if (m_fixtures.Contains(entity))
		{
			m_bodies[entity]->DestroyFixture(m_fixtures[entity]);
			m_fixtures[entity] = nullptr;
			m_fixtures.Erase(entity);
		}
	}
}

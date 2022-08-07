#include "Box2DPhysicsSystem.h"

#include "ECS/ECS.h"
#include "ECS/EntityView.h"
#include "Components/TransformComponent.h"
#include "MathHelpers.h"

namespace Puffin::Physics
{
	void Box2DPhysicsSystem::Init()
	{
		// Signature for all rigidbodies
		ECS::Signature rbSignature;
		rbSignature.set(m_world->GetComponentType<TransformComponent>());
		rbSignature.set(m_world->GetComponentType<Physics::Box2DRigidbodyComponent>());
		m_world->SetSystemSignature<Physics::Box2DPhysicsSystem>("Rigidbody", rbSignature);

		ECS::Signature circleSignature;
		circleSignature.set(m_world->GetComponentType<TransformComponent>());
		circleSignature.set(m_world->GetComponentType<Box2DCircleComponent>());
		m_world->SetSystemSignature<Box2DPhysicsSystem>("Circle", circleSignature);

		ECS::Signature boxSignature;
		boxSignature.set(m_world->GetComponentType<TransformComponent>());
		boxSignature.set(m_world->GetComponentType<Box2DBoxComponent>());
		m_world->SetSystemSignature<Box2DPhysicsSystem>("Box", boxSignature);

		// Register Events
		m_world->RegisterEvent<CollisionBeginEvent>();
		m_world->RegisterEvent<CollisionEndEvent>();
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
		m_physicsWorld->Step(m_fixedTime, m_velocityIterations, m_positionIterations);

		// Publish Collision Events
		PublishCollisionEvents();

		// Updated entity position/rotation from simulation

		for (ECS::EntityID entity : entityMap["Rigidbody"])
		{
			auto& transform = m_world->GetComponent<TransformComponent>(entity);
			auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

			transform.position.x = rb.body->GetPosition().x;
			transform.position.y = rb.body->GetPosition().y;
			transform.rotation.z = Maths::RadiansToDegrees(-rb.body->GetAngle());
		}

		/*ECS::EntityView<TransformComponent, Physics::Box2DRigidbodyComponent> rigidbodyView(m_world);

		for (ECS::Entity entity : rigidbodyView)
		{
			auto& transform = m_world->GetComponent<TransformComponent>(entity);
			auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

			transform.position.x = rb.body->GetPosition().x;
			transform.position.y = rb.body->GetPosition().y;
			transform.rotation.z = Maths::RadiansToDegrees(-rb.body->GetAngle());
		}*/
	}

	void Box2DPhysicsSystem::Stop()
	{
		// Cleanup Rigidbody Components
		for (ECS::EntityID entity : entityMap["Rigidbody"])
		{
			CleanupRigidbodyComponent(entity);
		}

		// Cleanup Box Components
		for (ECS::EntityID entity : entityMap["Box"])
		{
			CleanupBoxComponent(entity);
		}

		// Cleanup Box Components
		for (ECS::EntityID entity : entityMap["Circle"])
		{
			CleanupCircleComponent(entity);
		}

		m_circleShapes.Clear();
		m_polygonShapes.Clear();

		m_updateShapePointers = false;

		m_physicsWorld = nullptr;
		m_contactListener = nullptr;
	}

	void Box2DPhysicsSystem::PublishCollisionEvents() const
	{
		CollisionBeginEvent collisionBeginEvent;
		while (m_contactListener->GetNextCollisionBeginEvent(collisionBeginEvent))
		{
			m_world->PublishEvent(collisionBeginEvent);
		}

		CollisionEndEvent collisionEndEvent;
		while (m_contactListener->GetNextCollisionEndEvent(collisionEndEvent))
		{
			m_world->PublishEvent(collisionEndEvent);
		}
	}

	void Box2DPhysicsSystem::UpdateComponents()
	{
		// Update Rigidbody
		for (ECS::EntityID entity : entityMap["Rigidbody"])
		{
			if (m_world->GetComponentFlag<Box2DRigidbodyComponent, FlagDirty>(entity))
			{
				CleanupRigidbodyComponent(entity);
				InitRigidbodyComponent(entity);

				m_world->SetComponentFlag<Box2DRigidbodyComponent, FlagDirty>(entity, false);
			}

			if (m_world->GetComponentFlag<Box2DRigidbodyComponent, FlagDeleted>(entity))
			{
				CleanupRigidbodyComponent(entity);

				m_world->SetComponentFlag<Box2DRigidbodyComponent, FlagDeleted>(entity, false);
			}
		}
				
		// Update Box Components
		for (ECS::EntityID entity : entityMap["Box"])
		{
			auto& box = m_world->GetComponent<Box2DBoxComponent>(entity);

			if (m_world->GetComponentFlag<Box2DBoxComponent, FlagDirty>(entity))
			{
				CleanupBoxComponent(entity);

				if (m_world->HasComponent<Box2DRigidbodyComponent>(entity))
				{
					auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

					rb.fixture = nullptr;

					InitBoxForRigidbody(entity);
				}
				else
				{
					InitBoxComponent(entity);
				}

				m_world->SetComponentFlag<Box2DBoxComponent, FlagDirty>(entity, false);
			}

			if (m_world->GetComponentFlag<Box2DBoxComponent, FlagDeleted>(entity))
			{
				CleanupBoxComponent(entity);

				m_world->SetComponentFlag<Box2DBoxComponent, FlagDeleted>(entity, false);
			}

			if (!m_world->HasComponent<Box2DRigidbodyComponent>(entity))
			{
				if (m_updateShapePointers)
				{
					box.shape = &m_polygonShapes[entity];
				}
			}
		}

		// Update Circle Components
		for (ECS::EntityID entity : entityMap["Circle"])
		{
			auto& circle = m_world->GetComponent<Box2DCircleComponent>(entity);

			if (m_world->GetComponentFlag<Box2DCircleComponent, FlagDirty>(entity))
			{
				CleanupCircleComponent(entity);

				if (m_world->HasComponent<Box2DRigidbodyComponent>(entity))
				{
					auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

					rb.fixture = nullptr;

					InitCircleForRigidbody(entity);
				}
				else
				{
					InitCircleComponent(entity);
				}

				m_world->SetComponentFlag<Box2DCircleComponent, FlagDirty>(entity, false);
			}

			if (m_world->GetComponentFlag<Box2DCircleComponent, FlagDeleted>(entity))
			{
				CleanupCircleComponent(entity);

				m_world->SetComponentFlag<Box2DCircleComponent, FlagDeleted>(entity, false);
			}

			if (!m_world->HasComponent<Box2DRigidbodyComponent>(entity))
			{
				if (m_updateShapePointers)
				{
					circle.shape = &m_circleShapes[entity];
				}
			}
		}

		m_updateShapePointers = false;
	}

	void Box2DPhysicsSystem::InitRigidbodyComponent(ECS::EntityID entity)
	{
		const auto transform = m_world->GetComponent<TransformComponent>(entity);
		auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

		if (rb.body == nullptr)
		{
			b2BodyDef bodyDef = rb.bodyDef;
			bodyDef.userData.pointer = static_cast<uintptr_t>(entity);

			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = Maths::DegreesToRadians(-transform.rotation.z);

			// Created Body from Physics World
			rb.body = m_physicsWorld->CreateBody(&bodyDef);
		}
	}

	void Box2DPhysicsSystem::InitBoxForRigidbody(ECS::EntityID entity)
	{
		auto& box = m_world->GetComponent<Box2DBoxComponent>(entity);
		auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

		// Initialize Shape
		b2PolygonShape shape;
		shape.SetAsBox(box.data.halfExtent.x, box.data.halfExtent.y);

		b2FixtureDef fixtureDef = rb.fixtureDef;
		fixtureDef.shape = &shape;

		// Create Fixture between shape & rigidbody and store shape in component
		rb.fixture = rb.body->CreateFixture(&fixtureDef);
		box.shape = static_cast<b2PolygonShape*>(rb.fixture->GetShape());
	}

	void Box2DPhysicsSystem::InitCircleForRigidbody(ECS::EntityID entity)
	{
		auto& circle = m_world->GetComponent<Box2DCircleComponent>(entity);
		auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

		// Cleanup existing shape, if any
		CleanupCircleComponent(entity);

		// Initialize Shape
		b2CircleShape shape;
		shape.m_radius = circle.data.radius;

		// Create fixture def
		b2FixtureDef fixtureDef = rb.fixtureDef;
		fixtureDef.shape = &shape;

		// Create Fixture between shape & rigidbody and store shape in component
		rb.fixture = rb.body->CreateFixture(&fixtureDef);
		circle.shape = static_cast<b2CircleShape*>(rb.fixture->GetShape());
	}

	void Box2DPhysicsSystem::InitBoxComponent(ECS::EntityID entity)
	{
		auto& box = m_world->GetComponent<Box2DBoxComponent>(entity);
		auto& transform = m_world->GetComponent<TransformComponent>(entity);

		if (box.shape == nullptr)
		{
			b2PolygonShape shape;
			shape.SetAsBox(box.data.halfExtent.x, box.data.halfExtent.y, transform.position.GetXY(), Maths::DegreesToRadians(transform.rotation.z));

			m_polygonShapes.Insert(entity, shape);
			box.shape = &m_polygonShapes[entity];
		}
	}

	void Box2DPhysicsSystem::InitCircleComponent(ECS::EntityID entity)
	{
		auto& circle = m_world->GetComponent<Box2DCircleComponent>(entity);
		auto& transform = m_world->GetComponent<TransformComponent>(entity);

		if (circle.shape == nullptr)
		{
			b2CircleShape shape;
			shape.m_radius = circle.data.radius;
			shape.m_p.Set(transform.position.x, transform.position.y);

			m_circleShapes.Insert(entity, shape);
			circle.shape = &m_circleShapes[entity];
		}
	}

	void Box2DPhysicsSystem::CleanupRigidbodyComponent(ECS::EntityID entity)
	{
		auto& rb = m_world->GetComponent<Box2DRigidbodyComponent>(entity);

		rb.body = nullptr;
	}

	void Box2DPhysicsSystem::CleanupBoxComponent(ECS::EntityID entity)
	{
		auto& box = m_world->GetComponent<Box2DBoxComponent>(entity);

		if (box.shape != nullptr)
		{
			// Check if packed vector contains shape
			if (m_polygonShapes.Contains(entity))
			{
				m_polygonShapes.Erase(entity);
				m_updateShapePointers = true;
			}

			box.shape = nullptr;
		}
	}

	void Box2DPhysicsSystem::CleanupCircleComponent(ECS::EntityID entity)
	{
		auto& circle = m_world->GetComponent<Box2DCircleComponent>(entity);

		if (circle.shape != nullptr)
		{
			// Check if packed vector contains shape
			if (m_circleShapes.Contains(entity))
			{
				m_circleShapes.Erase(entity);
				m_updateShapePointers = true;
			}

			circle.shape = nullptr;
		}
	}
}

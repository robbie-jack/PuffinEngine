#pragma once

#include "box2d/box2d.h"
#include "box2d/b2_world.h"
#include "ECS/ECS.h"
#include "Types/PackedArray.h"
#include "Box2DContactListener.h"

namespace Puffin::Physics
{
	class Box2DPhysicsSystem : public ECS::System
	{
	public:

		Box2DPhysicsSystem()
		{
			m_systemInfo.name = "Box2DPhysicsSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::FixedUpdate;
		}

		~Box2DPhysicsSystem() override = default;

		void Init() override;
		void PreStart() override;
		void Start() override {}
		void Update() override;
		void Stop() override;
		void Cleanup() override {}

	private:

		b2Vec2 m_gravity = b2Vec2(0.0f, -9.81f);
		int32 m_velocityIterations = 8;
		int32 m_positionIterations = 3;

		std::unique_ptr<b2World> m_physicsWorld = nullptr;
		std::unique_ptr<Box2DContactListener> m_contactListener = nullptr;

		PackedVector<b2Body*> m_bodies; // Packed Vector of bodies use in physics simulation
		PackedVector<b2CircleShape> m_circleShapes; // Packed Vector of circle shapes
		PackedVector<b2PolygonShape> m_polygonShapes; // Packed Vector of polygon shapes
		PackedVector<b2Fixture*> m_fixtures; // Packed Vector of Fixtures that connect bodies and shapes

		void PublishCollisionEvents() const;
		void UpdateComponents();

		void InitRigidbodyComponent(ECS::EntityID entity);
		void InitBoxComponent(ECS::EntityID entity);
		void InitCircleComponent(ECS::EntityID entity);
		void InitFixture(ECS::EntityID entity, b2Shape* shape);

		void CleanupRigidbodyComponent(ECS::EntityID entity);
		void CleanupBoxComponent(ECS::EntityID entity);
		void CleanupCircleComponent(ECS::EntityID entity);
		void CleanupFixture(ECS::EntityID entity);
	};
}

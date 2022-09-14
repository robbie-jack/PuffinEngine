#pragma once

#include "box2d/box2d.h"
#include "box2d/b2_world.h"
#include "ECS/ECS.h"
#include "Components/Physics/Box2D/Box2DRigidbodyComponent.h"
#include "Components/Physics/Box2D/Box2DShapeComponents.h"
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

		PackedVector<ECS::EntityID, b2CircleShape> m_circleShapes; // Packed Vector of circle shapes which are not attached to a rigidbody
		PackedVector<ECS::EntityID, b2PolygonShape> m_polygonShapes; // Packed Vector of polygon shapes which are not attached to a rigidbody

		// Update shape components to point at correct shape in packed arrays
		bool m_updateShapePointers = false;

		void PublishCollisionEvents() const;
		void UpdateComponents();

		void InitRigidbodyComponent(ECS::EntityID entity);
		void InitBoxForRigidbody(ECS::EntityID entity);
		void InitCircleForRigidbody(ECS::EntityID entity);

		void InitBoxComponent(ECS::EntityID entity);
		void InitCircleComponent(ECS::EntityID entity);

		void CleanupRigidbodyComponent(ECS::EntityID entity);
		void CleanupBoxComponent(ECS::EntityID entity);
		void CleanupCircleComponent(ECS::EntityID entity);
	};
}

#pragma once

#include "Engine/Engine.hpp"
#include "ECS/EnTTSubsystem.hpp"
#include "Types/PackedArray.h"
#include "Box2DContactListener.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.hpp"

#include "box2d/box2d.h"
#include "box2d/b2_world.h"

namespace Puffin::Physics
{
	const inline std::unordered_map<BodyType, b2BodyType> G_BODY_TYPE_MAP =
	{
		{ BodyType::Static, b2_staticBody },
		{ BodyType::Kinematic, b2_kinematicBody },
		{ BodyType::Dynamic, b2_dynamicBody }
	};

	constexpr size_t G_MAX_SHAPES = 10000;

	class Box2DPhysicsSystem : public ECS::System
	{
	public:

		Box2DPhysicsSystem()
		{
			m_systemInfo.name = "Box2DPhysicsSystem";
		}

		~Box2DPhysicsSystem() override = default;

		void SetupCallbacks() override
		{
			m_engine->RegisterCallback(Core::ExecutionStage::Init, [&]() { Init(); }, "Box2DPhysicsSystem: Init");
			m_engine->RegisterCallback(Core::ExecutionStage::Start, [&]() { Start(); }, "Box2DPhysicsSystem: Start");
			m_engine->RegisterCallback(Core::ExecutionStage::FixedUpdate, [&]() { FixedUpdate(); }, "Box2DPhysicsSystem: FixedUpdate");
			m_engine->RegisterCallback(Core::ExecutionStage::Stop, [&]() { Stop(); }, "Box2DPhysicsSystem: Stop");

			auto registry = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->Registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent>>();

			registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnConstructBox>(this);
			//registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnConstructCircle>(this);
			//registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyCircle>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyRigidbody>(this);
		}

		void Init();
		void Start();
		void FixedUpdate();
		void Stop();

		void OnConstructBox(entt::registry& registry, entt::entity entity);
		void OnDestroyBox(entt::registry& registry, entt::entity entity);

		void OnConstructCircle(entt::registry& registry, entt::entity entity);
		void OnDestroyCircle(entt::registry& registry, entt::entity entity);

		void OnConstructRigidbody(entt::registry& registry, entt::entity entity);
		void OnDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		b2Vec2 m_gravity = b2Vec2(0.0f, -9.81f);
		int32 m_velocityIterations = 8;
		int32 m_positionIterations = 3;

		std::unique_ptr<b2World> m_physicsWorld = nullptr;
		std::unique_ptr<Box2DContactListener> m_contactListener = nullptr;

		PackedVector<b2Body*> m_bodies; // Packed Vector of bodies use in physics simulation
		PackedArray<b2Shape*, G_MAX_SHAPES> m_shapes;
		PackedArray<b2CircleShape, G_MAX_SHAPES> m_circleShapes; // Packed Vector of circle shapes
		PackedArray<b2PolygonShape, G_MAX_SHAPES> m_polygonShapes; // Packed Vector of polygon shapes
		PackedVector<b2Fixture*> m_fixtures; // Packed Vector of Fixtures that connect bodies and shapes

		std::vector<UUID> m_circlesToInit;
		std::vector<UUID> m_boxesToInit;
		std::vector<UUID> m_rigidbodiesToInit;

		void UpdateComponents();
		void PublishCollisionEvents() const;

		void InitRigidbodyComponent(UUID id, const TransformComponent& transform, const RigidbodyComponent2D& rb);
		void InitBoxComponent(UUID id, const TransformComponent& transform, const BoxComponent2D& box);
		void InitCircleComponent(UUID id, const TransformComponent& transform, const CircleComponent2D& circle);
		void InitFixture(UUID id, const RigidbodyComponent2D rb);

		void UpdateRigidbody(UUID id);
		void UpdateBox(UUID id);
		void UpdateCircle(UUID id);

		void CleanupRigidbodyComponent(UUID id);
		void CleanupBoxComponent(UUID id);
		void CleanupCircleComponent(UUID id);
		void CleanupFixture(UUID id);
	};
}

#pragma once

#include "Box2DContactListener.h"
#include "Core/Engine.h"
#include "ECS/EnTTSubsystem.h"
#include "Physics/PhysicsConstants.h"
#include "Types/PackedArray.h"
#include "Components/Physics/RigidbodyComponent2D.h"

#include "box2d/b2_world.h"
#include "box2d/box2d.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.h"

namespace puffin
{
	struct TransformComponent;
}

namespace puffin::physics
{

	const inline std::unordered_map<BodyType, b2BodyType> gBodyType =
	{
		{ BodyType::Static, b2_staticBody },
		{ BodyType::Kinematic, b2_kinematicBody },
		{ BodyType::Dynamic, b2_dynamicBody }
	};

	class Box2DPhysicsSystem : public core::System
	{
	public:

		Box2DPhysicsSystem()
		{
			mSystemInfo.name = "Box2DPhysicsSystem";
		}

		~Box2DPhysicsSystem() override = default;

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&]() { beginPlay(); }, "Box2DPhysicsSystem: BeginPlay");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&]() { fixedUpdate(); }, "Box2DPhysicsSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&]() { endPlay(); }, "Box2DPhysicsSystem: endPlay");

			auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent>>();

			registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSystem::onConstructBox>(this);
			//registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSystem::onDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSystem::onConstructCircle>(this);
			//registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSystem::onDestroyCircle>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::onDestroyRigidbody>(this);
		}

		void beginPlay();
		void fixedUpdate();
		void endPlay();

		void onConstructBox(entt::registry& registry, entt::entity entity);
		void onDestroyBox(entt::registry& registry, entt::entity entity);

		void onConstructCircle(entt::registry& registry, entt::entity entity);
		void onDestroyCircle(entt::registry& registry, entt::entity entity);

		void onConstructRigidbody(entt::registry& registry, entt::entity entity);
		void onDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		b2Vec2 mGravity = b2Vec2(0.0f, -9.81f);
		int32 mVelocityIterations = 8;
		int32 mPositionIterations = 3;

		std::unique_ptr<b2World> mPhysicsWorld = nullptr;
		std::unique_ptr<Box2DContactListener> mContactListener = nullptr;

		PackedVector<b2Body*> mBodies; // Packed Vector of bodies use in physics simulation
		PackedArray<b2Shape*, gMaxShapes> mShapes;
		PackedArray<b2CircleShape, gMaxShapes> mCircleShapes; // Packed Vector of circle shapes
		PackedArray<b2PolygonShape, gMaxShapes> mPolygonShapes; // Packed Vector of polygon shapes
		PackedVector<b2Fixture*> mFixtures; // Packed Vector of Fixtures that connect bodies and shapes

		std::vector<PuffinID> mCirclesToInit;
		std::vector<PuffinID> mBoxesToInit;
		std::vector<PuffinID> mRigidbodiesToInit;

		void updateComponents();
		void publishCollisionEvents() const;

		void initRigidbody(PuffinID id, const TransformComponent& transform, const RigidbodyComponent2D& rb);
		void initBox(PuffinID id, const TransformComponent& transform, const BoxComponent2D& box);
		void initCircle(PuffinID id, const TransformComponent& transform, const CircleComponent2D& circle);
		void initFixture(PuffinID id, const RigidbodyComponent2D rb);

		void updateRigidbody(PuffinID id);
		void updateBox(PuffinID id);
		void updateCircle(PuffinID id);

		void cleanupRigidbody(PuffinID id);
		void cleanupBox(PuffinID id);
		void cleanupCircle(PuffinID id);
		void cleanupFixture(PuffinID id);
	};
}

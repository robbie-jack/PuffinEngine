#pragma once

#include "ECS/Entity.hpp"
#include <ECS/ECS.h>
#include "Engine/Engine.hpp"
#include <Types/Vector.h>

#include <Physics/Onager2D/Shapes/BoxShape2D.h>
#include <Physics/Onager2D/Shapes/CircleShape2D.h>

#include "Physics/Onager2D/Broadphases/Broadphase2D.hpp"
#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Physics/Onager2D/PhysicsTypes2D.h"

#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.hpp"

#include "ECS/EnTTSubsystem.hpp"

#include "Types/PackedArray.h"

#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>


namespace puffin
{
	struct SceneObjectComponent;
}

namespace puffin::physics
{
	//////////////////////////////////////////////////
	// Physics System 2D
	//////////////////////////////////////////////////

	class Onager2DPhysicsSystem : public ECS::System
	{
	public:

		Onager2DPhysicsSystem();
		~Onager2DPhysicsSystem() override {}

		void SetupCallbacks() override
		{
			m_engine->registerCallback(Core::ExecutionStage::init, [&]() { init(); }, "Onager2DPhysicsSystem: Init");
			m_engine->registerCallback(Core::ExecutionStage::fixedUpdate, [&]() { fixedUpdate(); }, "Onager2DPhysicsSystem: FixedUpdate");
			m_engine->registerCallback(Core::ExecutionStage::stop, [&]() { stop(); }, "Onager2DPhysicsSystem: Stop");

			const auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent>>();

			registry->on_construct<BoxComponent2D>().connect<&Onager2DPhysicsSystem::onConstructBox>(this);
			registry->on_update<BoxComponent2D>().connect<&Onager2DPhysicsSystem::onConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&Onager2DPhysicsSystem::onDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&Onager2DPhysicsSystem::onConstructCircle>(this);
			registry->on_update<CircleComponent2D>().connect<&Onager2DPhysicsSystem::onConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&Onager2DPhysicsSystem::onDestroyCircle>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::onDestroyRigidbody>(this);
		}

		void init();
		void fixedUpdate();
		void stop();

		template<typename T>
		void registerBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(broadphases_.find(typeName) == broadphases_.end() && "Attempting to register already registered broadphase");

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->SetWorld(m_world);
			broadphaseBase->SetECS(m_engine->getSubsystem<ECS::EnTTSubsystem>());

			broadphases_.emplace(typeName, broadphaseBase);
		}

		template<typename T>
		void setBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(broadphases_.find(typeName) != broadphases_.end() && "Attempting to set un-registered broadphase");

			activeBroadphase_ = broadphases_[typeName];
		}

		void onConstructBox(entt::registry& registry, entt::entity entity);
		void onDestroyBox(entt::registry& registry, entt::entity entity);

		void onConstructCircle(entt::registry& registry, entt::entity entity);
		void onDestroyCircle(entt::registry& registry, entt::entity entity);

		void onConstructRigidbody(entt::registry& registry, entt::entity entity);
		void onDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		constexpr static size_t maxShapes = 10000;

		Vector2f gravity_ = Vector2f(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		PackedVector<Shape2D*> shapes_;
		PackedVector<BoxShape2D> boxShapes_;
		PackedVector<CircleShape2D> circleShapes_;
		PackedVector<std::shared_ptr<collision2D::Collider2D>> colliders_;

		bool collidersUpdated_ = false;

		std::vector<CollisionPair> collisionPairs_; // Pairs of entities which should be checked for collisions
		std::vector<collision2D::Contact> collisionContacts_; // Pairs of entities which have collided
		std::set<collision2D::Contact> activeContacts_; // Set for tracking active collisions

		std::shared_ptr<Broadphase> activeBroadphase_ = nullptr;
		std::unordered_map<const char*, std::shared_ptr<Broadphase>> broadphases_; // Map of registered broadphases

		// Init/Update/Delete of Physics Related Components
		void initCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle);
		void cleanupCircle(const SceneObjectComponent& object);

		void initBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box);
		void cleanupBox(const SceneObjectComponent& object);

		void insertCollider(UUID id, std::shared_ptr<collision2D::Collider2D> collider);
		void eraseCollider(UUID id);

		// Dynamics
		void updateDynamics() const; // Perform velocity updates for all rigid bodies
		void calculateImpulseByGravity(RigidbodyComponent2D& body) const; // Calculate Impulse due to force of gravity

		// Collision Broadphase
		void collisionBroadphase(); // Perform collision broadphase to decide which entities should collider together

		// Collision Detection
		void collisionDetection();

		// Resolve collisions found during collision detection, applying the correct Impulse 
		void collisionResponse() const;

		void generateCollisionEvents();

	};
}

#pragma once

#include "Broadphases/Broadphase2D.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Physics/VelocityComponent.h"
#include "ECS/EnTTSubsystem.h"
#include "Core/Engine.h"
#include "Core/System.h"
#include "Physics/PhysicsConstants.h"
#include "Physics/Onager2D/PhysicsTypes2D.h"
#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Physics/Onager2D/Shapes/BoxShape2D.h"
#include "Physics/Onager2D/Shapes/CircleShape2D.h"
#include "Types/PackedArray.h"
#include "Types/Vector.h"

#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>


namespace puffin
{
	struct SceneObjectComponent;
}

namespace puffin::physics
{
	//////////////////////////////////////////////////
	// Physics System 2D
	//////////////////////////////////////////////////

	class OnagerPhysicsSystem2D : public core::System
	{
	public:

		OnagerPhysicsSystem2D();
		~OnagerPhysicsSystem2D() override {}

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "Onager2DPhysicsSystem: Init");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&]() { fixedUpdate(); }, "Onager2DPhysicsSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&]() { endPlay(); }, "Onager2DPhysicsSystem: EndPlay");

			const auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent>>();

			registry->on_construct<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructBox>(this);
			registry->on_update<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructCircle>(this);
			registry->on_update<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyCircle>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&OnagerPhysicsSystem2D::onDestroyRigidbody>(this);
		}

		void init();
		void fixedUpdate();
		void endPlay();

		template<typename T>
		void registerBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(mBroadphases.find(typeName) == mBroadphases.end() && "Attempting to register already registered broadphase");

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->setECS(mEngine->getSubsystem<ecs::EnTTSubsystem>());

			mBroadphases.emplace(typeName, broadphaseBase);
		}

		template<typename T>
		void setBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(mBroadphases.find(typeName) != mBroadphases.end() && "Attempting to set un-registered broadphase");

			mActiveBroadphase = mBroadphases[typeName];
		}

		void onConstructBox(entt::registry& registry, entt::entity entity);
		void onDestroyBox(entt::registry& registry, entt::entity entity);

		void onConstructCircle(entt::registry& registry, entt::entity entity);
		void onDestroyCircle(entt::registry& registry, entt::entity entity);

		void onConstructRigidbody(entt::registry& registry, entt::entity entity);
		void onDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		Vector2f mGravity = Vector2f(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		PackedVector<Shape2D*> mShapes;
		PackedVector<BoxShape2D> mBoxShapes;
		PackedVector<CircleShape2D> mCircleShapes;
		PackedVector<std::shared_ptr<collision2D::Collider2D>> mColliders;

		bool mCollidersUpdated = false;

		std::vector<CollisionPair> mCollisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<collision2D::Contact> mCollisionContacts; // Pairs of entities which have collided
		std::set<collision2D::Contact> mActiveContacts; // Set for tracking active collisions

		std::shared_ptr<Broadphase> mActiveBroadphase = nullptr;
		std::unordered_map<const char*, std::shared_ptr<Broadphase>> mBroadphases; // Map of registered broadphases

		// Init/Update/Delete of Physics Related Components
		void initCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle);
		void cleanupCircle(const SceneObjectComponent& object);

		void initBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box);
		void cleanupBox(const SceneObjectComponent& object);

		void insertCollider(PuffinID id, std::shared_ptr<collision2D::Collider2D> collider);
		void eraseCollider(PuffinID id);

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

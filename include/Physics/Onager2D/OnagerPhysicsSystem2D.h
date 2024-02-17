#pragma once

#if PFN_ONAGER2D_PHYSICS

#include "Broadphases/Broadphase2D.h"
#include "Components/Physics/2D/RigidbodyComponent2D.h"
#include "Components/Physics/2D/ShapeComponents2D.h"
#include "Components/Physics/2D/VelocityComponent2D.h"
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

		OnagerPhysicsSystem2D(const std::shared_ptr<core::Engine>& engine);
		~OnagerPhysicsSystem2D() override { mEngine = nullptr; }

		void startup();
		void fixedUpdate();
		void endPlay();

		template<typename T>
		void registerBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(mBroadphases.find(typeName) == mBroadphases.end() && "Attempting to register already registered broadphase");

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->setECS(mEngine->getSystem<ecs::EnTTSubsystem>());

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

#endif // PFN_ONAGER2D_PHYSICS
#pragma once

#if PFN_ONAGER2D_PHYSICS

#include "puffin/physics/onager2d/broadphases/broadphase_2d.h"
#include "puffin/components/physics/2d/rigidbody_component_2d.h"
#include "puffin/components/physics/2d/shape_components_2d.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/core/engine.h"
#include "puffin/core/system.h"
#include "puffin/physics/onager2d/physics_types_2d.h"
#include "puffin/physics/onager2d/colliders/collider_2d.h"
#include "puffin/types/vector.h"

#include <memory>
#include <set>
#include <unordered_map>
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

        PackedVector<puffin::PuffinID, Shape2D*> mShapes;
        PackedVector<puffin::PuffinID, BoxShape2D> mBoxShapes;
        PackedVector<puffin::PuffinID, CircleShape2D> mCircleShapes;
        PackedVector<puffin::PuffinID, std::shared_ptr<collision2D::Collider2D>> mColliders;

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

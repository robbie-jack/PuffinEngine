#pragma once

#if PFN_ONAGER2D_PHYSICS

#include "puffin/physics/onager2d/broadphases/broadphase2d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/shapecomponents2d.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/core/engine.h"
#include "puffin/core/subsystem.h"
#include "puffin/physics/onager2d/physicstypes2d.h"
#include "puffin/physics/onager2d/colliders/collider2d.h"
#include "puffin/types/vector.h"
#include "puffin/types/uuid.h"

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
		~OnagerPhysicsSystem2D() override { m_engine = nullptr; }

		void startup();
		void update_fixed();
		void endPlay();

		template<typename T>
		void registerBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(mBroadphases.find(typeName) == mBroadphases.end() && "Attempting to register already registered broadphase");

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->setECS(m_engine->get_system<ecs::EnTTSubsystem>());

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

        MappedVector<puffin::PuffinID, Shape2D*> mShapes;
        MappedVector<puffin::PuffinID, BoxShape2D> mBoxShapes;
        MappedVector<puffin::PuffinID, CircleShape2D> mCircleShapes;
        MappedVector<puffin::PuffinID, std::shared_ptr<collision2D::Collider2D>> mColliders;

		bool mCollidersUpdated = false;

		std::vector<CollisionPair> mCollisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<collision2D::Contact> mCollisionContacts; // Pairs of entities which have collided
		std::set<collision2D::Contact> mActiveContacts; // Set for tracking active collisions

		std::shared_ptr<Broadphase> mActiveBroadphase = nullptr;
		std::unordered_map<const char*, std::shared_ptr<Broadphase>> mBroadphases; // Map of registered broadphases

		// Init/Update/Delete of Physics Related Components
        void initCircle(const entt::entity& entity, const puffin::PuffinID& id, const CircleComponent2D& circle);
        void cleanupCircle(const puffin::PuffinID& id);

        void initBox(const entt::entity& entity, const puffin::PuffinID& id, const BoxComponent2D& box);
        void cleanupBox(const puffin::PuffinID& id);

        void insertCollider(const puffin::PuffinID& id, std::shared_ptr<collision2D::Collider2D> collider);
        void eraseCollider(const puffin::PuffinID& id);

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

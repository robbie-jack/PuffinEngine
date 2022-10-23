#pragma once

#include "ECS/Entity.h"
#include <ECS/ECS.h>
#include <Types/Vector.h>

#include <Physics/PuffinPhysics2D/Shapes/BoxShape2D.h>
#include <Physics/PuffinPhysics2D/Shapes/CircleShape2D.h>

#include "Physics/PuffinPhysics2D/Broadphases/Broadphase2D.hpp"
#include "Physics/PuffinPhysics2D/Colliders/Collider2D.h"
#include "Physics/PuffinPhysics2D/PhysicsTypes2D.h"

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponents2D.h>

#include "Types/PackedArray.h"

#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Puffin::Physics
{
	const uint32_t MAX_SHAPES_PER_TYPE = 128; // Maximum number of shapes of each type

	//////////////////////////////////////////////////
	// Physics System 2D
	//////////////////////////////////////////////////

	class PhysicsSystem2D : public ECS::System
	{
	public:

		PhysicsSystem2D();
		~PhysicsSystem2D() override {}

		void Init() override;
		void PreStart() override;
		void Start() override {}
		void Update() override;
		void Stop() override;
		void Cleanup() override {}

		template<typename T>
		void RegisterBroadphase()
		{
			const char* typeName = typeid(T).name();

			if (m_broadphases.count(typeName) == 1)
				return;

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->SetWorld(m_world);

			m_broadphases.insert({typeName, broadphaseBase});
		}

		template<typename T>
		void SetBroadphase()
		{
			const char* typeName = typeid(T).name();

			if (m_broadphases.count(typeName) == 0)
				return;

			m_activeBroadphase = m_broadphases[typeName];
		}

	private:

		Vector2f m_gravity = Vector2f(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		PackedVector<BoxShape2D> m_boxShapes;
		PackedVector<CircleShape2D> m_circleShapes;
		PackedVector<std::shared_ptr<Collision2D::Collider2D>> m_colliders;

		std::vector<CollisionPair> m_collisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<Collision2D::Contact> m_collisionContacts; // Pairs of entities which have collided
		std::set<Collision2D::Contact> m_activeContacts; // Set for tracking active collisions

		std::shared_ptr<Broadphase> m_activeBroadphase = nullptr;
		std::unordered_map<const char*, std::shared_ptr<Broadphase>> m_broadphases; // Map of registered broadphases

		void InitCircle2D(std::shared_ptr<ECS::Entity> entity);
		void InitBox2D(std::shared_ptr<ECS::Entity> entity);

		void CleanupCircle2D(std::shared_ptr<ECS::Entity> entity);
		void CleanupBox2D(std::shared_ptr<ECS::Entity> entity);

		// Perform Initialization/Updating/Deltion of Physics Related Components
		void UpdateComponents();

		/* Step Physics Simulation
		 * dt - delta time value passed in by engine
		 * */
		void Step();

		// Dynamics
		void UpdateDynamics() const; // Perform velocity updates for all rigid bodies
		void CalculateImpulseByGravity(RigidbodyComponent2D& body) const; // Calculate Impulse due to force of gravity

		// Collision Broadphase
		void CollisionBroadphase(); // Perform collision broadphase to decide which entities should collider together

		// Collision Detection
		void CollisionDetection();

		// Resolve collisions found during collision detection, applying the correct Impulse 
		void CollisionResponse() const;

		void GenerateCollisionEvents();

	};
}

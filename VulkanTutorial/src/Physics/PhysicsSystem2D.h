#pragma once

#include <ECS/ECS.h>
#include <Types/Vector.h>

#include <Physics/Shapes/BoxShape2D.h>
#include <Physics/Shapes/CircleShape2D.h>

#include "Physics/Colliders/Collider2D.h"
#include "Physics/PhysicsTypes2D.h"

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponents2D.h>

#include "Types/PackedArray.h"

#include <utility>
#include <vector>

namespace Puffin::Physics
{
	const uint32_t MAX_SHAPES_PER_TYPE = 128; // Maximum number of shapes of each type

	typedef std::pair<const std::shared_ptr<Collision2D::Collider2D>, const std::shared_ptr<Collision2D::Collider2D>> CollisionPair;

	//////////////////////////////////////////////////
	// Physics System 2D
	//////////////////////////////////////////////////

	class PhysicsSystem2D : public ECS::System
	{
	public:

		PhysicsSystem2D();
		~PhysicsSystem2D() {};

		void Init() override {};
		void PreStart() override;
		void Start() override {};
		void Update() override;
		void Stop() override;
		void Cleanup() override {};

		ECS::SystemInfo GetInfo() override
		{
			ECS::SystemInfo info;

			info.updateOrder = ECS::UpdateOrder::None;

			return info;
		}

	private:

		Vector2f m_gravity = Vector2f(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		PackedVector<ECS::Entity, BoxShape2D> m_boxShapes;
		PackedVector<ECS::Entity, CircleShape2D> m_circleShapes;
		PackedVector<ECS::Entity, std::shared_ptr<Collision2D::Collider2D>> m_colliders;

		std::vector<CollisionPair> m_collisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<Collision2D::Contact> m_collisionContacts; // Pairs of entities which have collided

		void InitCircle2D(ECS::Entity entity, CircleComponent2D& circle);
		void InitBox2D(ECS::Entity entity, BoxComponent2D& box);

		void CleanupCircle2D(ECS::Entity entity, CircleComponent2D& circle);
		void CleanupBox2D(ECS::Entity entity, BoxComponent2D& circle);

		// Perform Initialization/Updating/Deltion of Physics Related Components
		void UpdateComponents();

		/* Step Physics Simulation
		 * dt - delta time value passed in by engine
		 * */
		void Step();

		// Dynamics
		void UpdateDynamics(); // Perform velocity updates for all rigid bodies
		void CalculateImpulseByGravity(RigidbodyComponent2D& body); // Calculate Impulse due to force of gravity

		// Collision Broadphase
		void CollisionBroadphase(); // Perform collision broadphase to decide which entities should collider together
		void GenerateCollisionPairs(); // Generate collision pairs using the N_Squared Broadphase

		// Collision Detection
		void CollisionDetection();

		// Resolve collisions found during collision detection, applying the correct Impulse 
		void CollisionResponse();

	};

}

#pragma once

#include <ECS/ECS.h>
#include <Types/Vector.h>

#include <Physics/Shapes/BoxShape2D.h>
#include <Physics/Shapes/CircleShape2D.h>

#include "Physics/Colliders/Collider2D.h"

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponents2D.h>

#include "Physics/PhysicsTypes2D.h"

#include <utility>
#include <vector>

namespace Puffin::Physics
{
	const uint32_t MAX_SHAPES_PER_TYPE = 128; // Maximum number of shapes of each type

	typedef std::pair<const Collision2D::Collider2D*, const Collision2D::Collider2D*> CollisionPair;

	//////////////////////////////////////////////////
	// Physics System 2D
	//////////////////////////////////////////////////

	class PhysicsSystem2D : public ECS::System
	{
	public:

		void Init(float inTimeStep = 1 / 60.0f);
		void Update(float dt);
		void Cleanup();

	private:

		Vector2 gravity = Vector2(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		float timeStep = 1.0f / 60.0f; // How often the physics world will update, defaults to 60 times a second
		float accumulatedTime = 0.0f; // Time Accumulated Since last Physics Step

		std::vector<BoxShape2D> boxShapes_;
		std::vector<CircleShape2D> circleShapes_;
		std::vector<Collision2D::Collider2D*> colliders_;

		std::vector<CollisionPair> collisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<Collision2D::Contact> collisionContacts; // Pairs of entities which have collided

		void UpdateComponents();

		void Step(float dt);

		// Dynamics
		void UpdateDynamics();
		void CalculateImpulseByGravity(RigidbodyComponent2D& Body, const float& dt);

		// Collision Broadphase
		void CollisionBroadphase();
		void GenerateCollisionPairs();

		// Collision Detection
		void CollisionDetection();

		// Collision Resolve
		void CollisionResolve();

	};

}

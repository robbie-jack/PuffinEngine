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


namespace Puffin
{
	struct SceneObjectComponent;
}

namespace Puffin::Physics
{
	const uint32_t MAX_SHAPES_PER_TYPE = 128; // Maximum number of shapes of each type

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
			m_engine->RegisterCallback(Core::ExecutionStage::Init, [&]() { Init(); }, "Onager2DPhysicsSystem: Init");
			m_engine->RegisterCallback(Core::ExecutionStage::FixedUpdate, [&]() { FixedUpdate(); }, "Onager2DPhysicsSystem: FixedUpdate");
			m_engine->RegisterCallback(Core::ExecutionStage::Stop, [&]() { Stop(); }, "Onager2DPhysicsSystem: Stop");

			auto registry = m_engine->GetSubsystem<ECS::EnTTSubsystem>()->Registry();

			registry->on_construct<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::OnDestroyRigidbody>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent>>();
			registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent>>();

			registry->on_construct<BoxComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructBox>(this);
			registry->on_update<BoxComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructBox>(this);
			registry->on_destroy<BoxComponent2D>().connect<&Onager2DPhysicsSystem::OnDestroyBox>(this);

			registry->on_construct<CircleComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructCircle>(this);
			registry->on_update<CircleComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructCircle>(this);
			registry->on_destroy<CircleComponent2D>().connect<&Onager2DPhysicsSystem::OnDestroyCircle>(this);

			registry->on_construct<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::OnConstructRigidbody>(this);
			registry->on_destroy<RigidbodyComponent2D>().connect<&Onager2DPhysicsSystem::OnDestroyRigidbody>(this);
		}

		void Init();
		void FixedUpdate();
		void Stop();

		template<typename T>
		void RegisterBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(m_broadphases.count(typeName) == 0 && "Attempting to register already registered broadphase");

			std::shared_ptr<T> broadphase = std::make_shared<T>();
			std::shared_ptr<Broadphase> broadphaseBase = std::static_pointer_cast<Broadphase>(broadphase);
			broadphaseBase->SetWorld(m_world);
			broadphaseBase->SetECS(m_engine->GetSubsystem<ECS::EnTTSubsystem>());

			m_broadphases.insert({typeName, broadphaseBase});
		}

		template<typename T>
		void SetBroadphase()
		{
			const char* typeName = typeid(T).name();

			assert(m_broadphases.count(typeName) == 1 && "Attempting to set un-registered broadphase");

			m_activeBroadphase = m_broadphases[typeName];
		}

		void OnConstructBox(entt::registry& registry, entt::entity entity);
		void OnDestroyBox(entt::registry& registry, entt::entity entity);

		void OnConstructCircle(entt::registry& registry, entt::entity entity);
		void OnDestroyCircle(entt::registry& registry, entt::entity entity);

		void OnConstructRigidbody(entt::registry& registry, entt::entity entity);
		void OnDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		Vector2f m_gravity = Vector2f(0.0f, -9.81f); // Global Gravity value which gets applied to dynamic objects each physics step

		PackedVector<Shape2D*> m_shapes;
		PackedVector<BoxShape2D> m_boxShapes;
		PackedVector<CircleShape2D> m_circleShapes;
		PackedVector<std::shared_ptr<Collision2D::Collider2D>> m_colliders;

		bool m_collidersUpdated = false;

		std::vector<CollisionPair> m_collisionPairs; // Pairs of entities which should be checked for collisions
		std::vector<Collision2D::Contact> m_collisionContacts; // Pairs of entities which have collided
		std::set<Collision2D::Contact> m_activeContacts; // Set for tracking active collisions

		std::shared_ptr<Broadphase> m_activeBroadphase = nullptr;
		std::unordered_map<const char*, std::shared_ptr<Broadphase>> m_broadphases; // Map of registered broadphases

		// Init/Update/Delete of Physics Related Components
		void InitCircle(const entt::entity& entity, const SceneObjectComponent& object, const CircleComponent2D& circle);
		void CleanupCircle(const SceneObjectComponent& object);

		void InitBox(const entt::entity& entity, const SceneObjectComponent& object, const BoxComponent2D& box);
		void CleanupBox(const SceneObjectComponent& object);

		void InsertCollider(UUID id, std::shared_ptr<Collision2D::Collider2D> collider);
		void EraseCollider(UUID id);

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

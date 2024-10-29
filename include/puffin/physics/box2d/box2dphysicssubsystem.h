#pragma once

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/subsystem.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/physics/physicsconstants.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/physics/bodytype.h"
#include "puffin/types/storage/ringbuffer.h"

#include "box2d/box2d.h"

namespace puffin
{
	struct TransformComponent2D;
	struct TransformComponent3D;

	namespace core
	{
		class Engine;
	}
}

namespace puffin::physics
{
	struct RigidbodyComponent2D;
	struct BoxComponent2D;
	struct CircleComponent2D;

	const inline std::unordered_map<BodyType, b2BodyType> gPuffinToBox2DBodyType =
	{
		{ BodyType::Static, b2_staticBody },
		{ BodyType::Kinematic, b2_kinematicBody },
		{ BodyType::Dynamic, b2_dynamicBody }
	};

	class Box2DPhysicsSystem : public core::Subsystem
	{
	public:

		explicit Box2DPhysicsSystem(const std::shared_ptr<core::Engine>& engine);
		~Box2DPhysicsSystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		void BeginPlay() override;
		void EndPlay() override;

		void FixedUpdate(double fixedTimeStep) override;
		bool ShouldFixedUpdate() override;

		void OnConstructBox(entt::registry& registry, entt::entity entity);
		void OnUpdateBox(entt::registry& registry, entt::entity entity);
		void OnDestroyBox(entt::registry& registry, entt::entity entity);

		void OnConstructCircle(entt::registry& registry, entt::entity entity);
		void OnUpdateCircle(entt::registry& registry, entt::entity entity);
		void OnDestroyCircle(entt::registry& registry, entt::entity entity);

		void OnConstructRigidbody(entt::registry& registry, entt::entity entity);
		void OnUpdateRigidbody(entt::registry& registry, entt::entity entity);
		void OnDestroyRigidbody(entt::registry& registry, entt::entity entity);

	private:

		void InitSettingsAndSignals();
		
		void CreateObjects();
		void DestroyObjects();
		void PublishCollisionEvents() const;

		void CreateBody(UUID id, const TransformComponent2D& transform, const RigidbodyComponent2D& rb);
		void CreateBox(UUID id, const TransformComponent2D& transform, const BoxComponent2D& box);
		void CreateCircle(UUID id, const TransformComponent2D& transform, const CircleComponent2D& circle);

		void UpdateBody(UUID id);
		void UpdateBox(UUID id);
		void UpdateCircle(UUID id);

		void DestroyBody(UUID id);
		void DestroyBox(UUID id);
		void DestroyCircle(UUID id);
		
		struct BodyCreateEvent
		{
			UUID id;
		};

		struct ShapeCreateEvent
		{
			UUID id;
			BodyType type;
		};

		bool mEnabled = false;
		b2Vec2 mGravity = { 0.0, 0.0 };
		int mSubSteps = 0;

		b2WorldId mPhysicsWorldID = b2_nullWorldId;
		//std::unique_ptr<Box2DContactListener> m_contact_listener = nullptr;

		MappedVector<UUID, b2BodyId> mBodyIDs; // Vector of body ids used in physics simulation
		MappedVector<UUID, b2ShapeId> mShapeIDs; // Vector of shapes sued in physics simulation

		MappedVector<UUID, b2Circle> mCircleIDs;
		MappedVector<UUID, b2Polygon> mPolygonIDs;

		RingBuffer<BodyCreateEvent> mBodyCreateEvents;
		RingBuffer<ShapeCreateEvent> mShapeCreateEvents;
	};
}

#endif // PFN_BOX2D_PHYSICS
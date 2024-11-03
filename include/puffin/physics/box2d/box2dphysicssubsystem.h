#pragma once

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/subsystem.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/physics/shapetype2d.h"
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

		void CreateBody(entt::entity entity, UUID id);
		void CreateBox(entt::entity entity, UUID boxUUID, UUID bodyUUID);
		void CreateCircle(entt::entity entity, UUID circleUUID, UUID bodyUUID);

		void UpdateBody(entt::entity entity, UUID id);
		void UpdateBox(entt::entity entity, UUID id);
		void UpdateCircle(entt::entity entity, UUID id);

		void DestroyBody(entt::entity entity, UUID id);
		void DestroyBox(entt::entity entity, UUID id);
		void DestroyCircle(entt::entity entity, UUID id);
		
		struct PhysicsEvent
		{
			entt::entity entity;
			UUID id;
		};

		using BodyCreateEvent = PhysicsEvent;
		using BodyDestroyEvent = PhysicsEvent;

		struct ShapeEvent : PhysicsEvent
		{
			ShapeType2D shapeType;
		};

		using ShapeCreateEvent = ShapeEvent;
		using ShapeDestroyEvent = ShapeEvent;

		struct UserData
		{
			UUID id;
			entt::entity entity;
			
		};

		struct BodyData
		{
			b2BodyId bodyID;
			std::unordered_set<UUID> shapeIDs;
		};

		struct ShapeData
		{
			b2ShapeId shapeID;
			ShapeType2D shapeType;
		};

		bool mEnabled = false;
		b2Vec2 mGravity = { 0.0, 0.0 };
		int mSubSteps = 0;

		b2WorldId mPhysicsWorldID = b2_nullWorldId;
		//std::unique_ptr<Box2DContactListener> m_contact_listener = nullptr;

		std::unordered_map<UUID, UserData> mUserData;
		std::unordered_map<UUID, BodyData> mBodyData; // Vector of body ids used in physics simulation
		std::unordered_map<UUID, ShapeData> mShapeData; // Vector of shapes used in physics simulation

		RingBuffer<BodyCreateEvent> mBodyCreateEvents;
		RingBuffer<BodyDestroyEvent> mBodyDestroyEvents;
		
		RingBuffer<ShapeCreateEvent> mShapeCreateEvents;
		RingBuffer<ShapeDestroyEvent> mShapeDestroyEvents;
	};
}

#endif // PFN_BOX2D_PHYSICS
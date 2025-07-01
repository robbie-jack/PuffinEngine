#pragma once

#if PFN_BOX2D_PHYSICS

#include "TaskScheduler.h"

#include "core/engine.h"
#include "subsystem/gameplay_subsystem.h"
#include "ecs/entt_subsystem.h"
#include "physics/shape_type_2d.h"
#include "physics/physics_constants.h"
#include "types/storage/mapped_vector.h"
#include "physics/body_type.h"
#include "types/storage/ring_buffer.h"

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

namespace puffin
{
	namespace physics
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

		class Box2DTask : public enki::ITaskSet
		{
		public:

			Box2DTask() = default;

			void ExecuteRange(enki::TaskSetPartition range, uint32_t threadIndex) override
			{
				mTask(range.start, range.end, threadIndex, mTaskContext);
			}

			b2TaskCallback* mTask = nullptr;
			void* mTaskContext = nullptr;
		};

		constexpr int32_t gMaxTasks = 64;
		constexpr int32_t gMaxThreads = 64;

		class Box2DPhysicsSubsystem : public core::GameplaySubsystem
		{
		public:

			explicit Box2DPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine);
			~Box2DPhysicsSubsystem() override = default;

			void PreInitialize(core::SubsystemManager* subsystemManager) override;
			void Initialize() override;
			void Deinitialize() override;

			void BeginPlay() override;
			void EndPlay() override;

			void FixedUpdate(double fixedTimeStep) override;
			bool ShouldFixedUpdate() override;

			std::string_view GetName() const override;

			int32_t& TaskCount() { return mTaskCount; }
			std::array<Box2DTask, gMaxTasks>& Tasks() { return mTasks; }
			enki::TaskScheduler& TaskScheduler();

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

			void InitConnections();
			void InitSettingsAndSignals();

			void CreateObjects();
			void DestroyObjects();
			void PublishCollisionEvents() const;

			void UpdateRigidbodyNodes();
			void UpdateRigidbodyComponents();

			void CreateBodyComponent(UUID id);
			void CreateBoxComponent(UUID boxId, UUID bodyId);
			void CreateCircleComponent(UUID circleId, UUID bodyId);

			void CreateBodyNode(UUID id);
			void CreateBoxNode(UUID boxId, UUID bodyId);
			void CreateCircleNode(UUID circleId, UUID bodyId);

			void DestroyBody(UUID id);
			void DestroyBox(UUID id);
			void DestroyCircle(UUID id);

			struct PhysicsEvent
			{
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

			std::array<Box2DTask, gMaxTasks> mTasks;
			int32_t mTaskCount = 0;

			std::unordered_map<UUID, UserData> mUserData;
			std::unordered_map<UUID, BodyData> mBodyData; // Vector of body ids used in physics simulation
			std::unordered_map<UUID, ShapeData> mShapeData; // Vector of shapes used in physics simulation

			RingBuffer<BodyCreateEvent> mBodyCreateEvents;
			RingBuffer<BodyDestroyEvent> mBodyDestroyEvents;

			RingBuffer<ShapeCreateEvent> mShapeCreateEvents;
			RingBuffer<ShapeDestroyEvent> mShapeDestroyEvents;

			std::vector<entt::connection> mConnections;
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::Box2DPhysicsSubsystem>()
		{
			return "Box2DPhysicsSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<physics::Box2DPhysicsSubsystem>()
		{
			return entt::hs(GetTypeString<physics::Box2DPhysicsSubsystem>().data());
		}

		template<>
		inline void RegisterType<physics::Box2DPhysicsSubsystem>()
		{
			auto meta = entt::meta<physics::Box2DPhysicsSubsystem>()
				.base<core::GameplaySubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}

#endif // PFN_BOX2D_PHYSICS
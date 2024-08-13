#pragma once

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/subsystem.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/physics/physics_constants.h"
#include "puffin/types/packed_array.h"
#include "puffin/components/transform_component_2d.h"
#include "puffin/components/physics/2d/rigidbody_component_2d.h"
#include "puffin/components/physics/2d/shape_components_2d.h"
#include "puffin/components/physics/2d/velocity_component_2d.h"
#include "puffin/types/packed_vector.h"
#include "puffin/types/ring_buffer.h"

#include "box2d/box2d.h"

namespace puffin
{
	struct TransformComponent3D;

	namespace core
	{
		class Engine;
	}
}

namespace puffin::physics
{

	const inline std::unordered_map<BodyType, b2BodyType> g_puffin_to_box2d_body_type =
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

		void initialize(core::SubsystemManager* subsystem_manager) override;
		void deinitialize() override;

		core::SubsystemType type() const override;

		void begin_play() override;
		void end_play() override;

		void update(double delta_time) override;
		bool should_update() override;

		void fixed_update(double fixed_time_step) override;
		bool should_fixed_update() override;

		void on_construct_box(entt::registry& registry, entt::entity entity);
		void on_update_box(entt::registry& registry, entt::entity entity);
		void on_destroy_box(entt::registry& registry, entt::entity entity);

		void on_construct_circle(entt::registry& registry, entt::entity entity);
		void on_update_circle(entt::registry& registry, entt::entity entity);
		void on_destroy_circle(entt::registry& registry, entt::entity entity);

		void on_construct_rigidbody(entt::registry& registry, entt::entity entity);
		void on_update_rigidbody(entt::registry& registry, entt::entity entity);
		void on_destroy_rigidbody(entt::registry& registry, entt::entity entity);

	private:

		struct BodyCreateEvent
		{
			PuffinID id;
		};

		struct ShapeCreateEvent
		{
			PuffinID id;
			BodyType type;
		};

		uint32_t m_sub_steps = 0;

		b2WorldId m_physics_world_id = b2_nullWorldId;
		//std::unique_ptr<Box2DContactListener> m_contact_listener = nullptr;

		PackedVector<PuffinID, b2BodyId> m_body_ids; // Vector of body ids used in physics simulation
		PackedVector<PuffinID, b2ShapeId> m_shapes; // Vector of shapes sued in physics simulation

		PackedVector<PuffinID, b2Circle> m_circles;
		PackedVector<PuffinID, b2Polygon> m_polygons;

		RingBuffer<BodyCreateEvent> m_body_create_events;
		RingBuffer<ShapeCreateEvent> m_shape_create_events;

		void create_objects();
		void destroy_objects();
		void publish_collision_events() const;

		void create_body(PuffinID id, const TransformComponent2D& transform, const RigidbodyComponent2D& rb);
		void create_box(PuffinID id, const TransformComponent2D& transform, const BoxComponent2D& box);
		void create_circle(PuffinID id, const TransformComponent2D& transform, const CircleComponent2D& circle);

		void update_body(PuffinID id);
		void update_box(PuffinID id);
		void update_circle(PuffinID id);

		void destroy_body(PuffinID id);
		void destroy_box(PuffinID id);
		void destroy_circle(PuffinID id);
	};
}

#endif // PFN_BOX2D_PHYSICS
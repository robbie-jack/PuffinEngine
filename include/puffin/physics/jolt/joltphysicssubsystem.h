#pragma once

#if PFN_JOLT_PHYSICS

#ifdef PFN_DOUBLE_PRECISION
#define JPH_DOUBLE_PRECISION 1
#endif

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED 1;
#define JPH_PROFILE_ENABLED 1;
#define JPH_DEBUG_RENDERER 1;

#include "puffin/physics/jolt/jolt_physics_types.h"

#include "Jolt/Jolt.h"

#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "puffin/core/subsystem.h"
#include "puffin/components/physics/3d/rigidbody_component_3d.h"
#include "puffin/components/physics/3d/shape_components_3d.h"
#include "puffin/components/physics/3d/velocity_component_3d.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/physics/body_type.h"
#include "puffin/physics/physics_constants.h"
#include "puffin/types/packed_vector.h"

namespace puffin::physics
{
	const inline std::unordered_map<BodyType, JPH::EMotionType> g_jolt_body_type =
	{
		{ BodyType::Static, JPH::EMotionType::Static },
		{ BodyType::Kinematic, JPH::EMotionType::Kinematic },
		{ BodyType::Dynamic, JPH::EMotionType::Dynamic }
	};

	class JoltPhysicsSubsystem : public core::Subsystem
	{
	public:

		JoltPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine);
		~JoltPhysicsSubsystem() override = default;

		void initialize(core::SubsystemManager* subsystem_manager) override;
		void deinitialize() override;

		core::SubsystemType type() const override;

		void begin_play() override;
		void end_play() override;

		void fixed_update(double fixed_time) override;
		bool should_fixed_update() override;

		void on_construct_box(entt::registry& registry, entt::entity entity);
		void on_destroy_box(entt::registry& registry, entt::entity entity);

		void on_construct_sphere(entt::registry& registry, entt::entity entity);
		void on_destroy_sphere(entt::registry& registry, entt::entity entity);

		void on_construct_rigidbody(entt::registry& registry, entt::entity entity);
		void on_destroy_rigidbody(entt::registry& registry, entt::entity entity);

		void update_time_step();

	private:

		JPH::Vec3Arg m_gravity = JPH::Vec3Arg(0.0, -9.81, 0.0);
		double m_fixed_time_step = 0.0;
		const double m_ideal_time_step = 1 / 60.0; // Ideal time step of physics simulation
		int m_collision_steps = 1; // Number of collision steps doen in physics simulation. Defualts to one, will be set higher if time step is greater than 1 / 60

		const JPH::uint m_num_body_mutexes = 0;
		const JPH::uint m_max_body_pairs = 65536;
		const JPH::uint m_max_contact_constraints = 10240;

		std::unique_ptr<JPH::PhysicsSystem> m_internal_physics_system;
		std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator;
		std::unique_ptr<JPH::JobSystemThreadPool> m_job_system;

		JoltBPLayerInterfaceImpl m_bp_layer_interface_impl;
		JoltObjectLayerPairFilterImpl m_object_vs_object_layer_filter;
		JoltObjectVsBroadPhaseLayerFilterImpl m_object_vs_broadphase_layer_filter;

		PackedVector<PuffinID, JPH::ShapeRefC> m_shape_refs;
		PackedVector<PuffinID, JPH::Body*> m_bodies;

		std::vector<PuffinID> m_boxes_to_init;
		std::vector<PuffinID> m_spheres_to_init;
		std::vector<PuffinID> m_bodies_to_init;

		std::vector<PuffinID> m_bodies_to_add;

		void update_components();

		void init_box(PuffinID id, const TransformComponent3D& transform, const BoxComponent3D& box);
		void init_sphere(PuffinID id, const TransformComponent3D& transform, const SphereComponent3D& circle);
		void init_rigidbody(PuffinID id, const TransformComponent3D& transform, const RigidbodyComponent3D& rb);
	};
}

#endif // PFN_JOLT_PHYSICS
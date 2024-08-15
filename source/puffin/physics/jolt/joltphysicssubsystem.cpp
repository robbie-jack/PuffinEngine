#include "puffin/physics/jolt/joltphysicssubsystem.h"

#if PFN_JOLT_PHYSICS

#include <iostream>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>

namespace puffin::physics
{
	JoltPhysicsSubsystem::JoltPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		m_name = "JoltPhysicsSubsystem";
	}

	void JoltPhysicsSubsystem::initialize(core::SubsystemManager* subsystem_manager)
	{
		auto entt_subsystem = m_engine->get_subsystem<ecs::EnTTSubsystem>();
		auto registry = entt_subsystem->registry();

		registry->on_construct<RigidbodyComponent3D>().connect<&JoltPhysicsSubsystem::on_construct_rigidbody>(this);
		registry->on_destroy<RigidbodyComponent3D>().connect<&JoltPhysicsSubsystem::on_destroy_rigidbody>(this);

		registry->on_construct<RigidbodyComponent3D>().connect<&entt::registry::emplace<VelocityComponent3D>>();
		registry->on_destroy<RigidbodyComponent3D>().connect<&entt::registry::remove<VelocityComponent3D>>();

		registry->on_construct<BoxComponent3D>().connect<&JoltPhysicsSubsystem::on_construct_box>(this);
		//registry->on_update<BoxComponent3D>().connect<&JoltPhysicsSystem::OnConstructBox>(this);
		registry->on_destroy<BoxComponent3D>().connect<&JoltPhysicsSubsystem::on_destroy_box>(this);

		registry->on_construct<SphereComponent3D>().connect<&JoltPhysicsSubsystem::on_construct_sphere>(this);
		//registry->on_update<SphereComponent3D>().connect<&JoltPhysicsSystem::onConstructSphere>(this);
		registry->on_destroy<SphereComponent3D>().connect<&JoltPhysicsSubsystem::on_destroy_sphere>(this);

		m_shape_refs.reserve(gMaxShapes);

		update_time_step();
	}

	void JoltPhysicsSubsystem::deinitialize()
	{
	}

	core::SubsystemType JoltPhysicsSubsystem::type() const
	{
		return core::SubsystemType::Gameplay;
	}

	void JoltPhysicsSubsystem::begin_play()
	{
		// Register allocation hook
		JPH::RegisterDefaultAllocator();

		// Create factory
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all Jolt physics types
		JPH::RegisterTypes();

		m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		m_job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		m_internal_physics_system = std::make_unique<JPH::PhysicsSystem>();
		m_internal_physics_system->Init(gMaxShapes, m_num_body_mutexes, m_max_body_pairs, m_max_contact_constraints,
			m_bp_layer_interface_impl, m_object_vs_broadphase_layer_filter, m_object_vs_object_layer_filter);

		m_internal_physics_system->SetGravity(m_gravity);

		update_components();
	}

	void JoltPhysicsSubsystem::end_play()
	{
		m_bodies.clear();
		m_shape_refs.clear();

		m_bodies_to_add.clear();
		m_bodies_to_init.clear();
		m_boxes_to_init.clear();
		m_spheres_to_init.clear();

		m_internal_physics_system = nullptr;
		m_job_system = nullptr;
		m_temp_allocator = nullptr;

		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void JoltPhysicsSubsystem::fixed_update(double fixed_time)
	{
		update_components();

		m_internal_physics_system->Update(m_fixed_time_step, m_collision_steps, m_temp_allocator.get(), m_job_system.get());

		const auto registry = m_engine->get_subsystem<ecs::EnTTSubsystem>()->registry();

		// Updated entity position/rotation from simulation
		const auto bodyView = registry->view<TransformComponent3D, VelocityComponent3D, const RigidbodyComponent3D>();

		for (auto [entity, transform, velocity, rb] : bodyView.each())
		{
			const auto& id = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_id(entity);

			// Update Transform from Rigidbody Position
			registry->patch<TransformComponent3D>(entity, [&](auto& transform)
			{
				transform.position.x = m_bodies[id]->GetCenterOfMassPosition().GetX();
				transform.position.y = m_bodies[id]->GetCenterOfMassPosition().GetY();
				transform.position.z = m_bodies[id]->GetCenterOfMassPosition().GetZ();
				//transform.rotation = maths::radToDeg(-mBodies[id]->GetAngle());
			});

			// Update Velocity with Linear/Angular Velocity#
			registry->patch<VelocityComponent3D>(entity, [&](auto& velocity)
			{
				velocity.linear.x = m_bodies[id]->GetLinearVelocity().GetX();
				velocity.linear.y = m_bodies[id]->GetLinearVelocity().GetY();
				velocity.linear.z = m_bodies[id]->GetLinearVelocity().GetZ();
			});
		}
	}

	bool JoltPhysicsSubsystem::should_fixed_update()
	{
		return true;
	}

	void JoltPhysicsSubsystem::on_construct_box(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_id(entity);

		m_boxes_to_init.push_back(id);
	}

	void JoltPhysicsSubsystem::on_destroy_box(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::on_construct_sphere(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_id(entity);

		m_spheres_to_init.push_back(id);
	}

	void JoltPhysicsSubsystem::on_destroy_sphere(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::on_construct_rigidbody(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_id(entity);

		m_bodies_to_init.push_back(id);
	}

	void JoltPhysicsSubsystem::on_destroy_rigidbody(entt::registry& registry, entt::entity entity)
	{

	}

	void JoltPhysicsSubsystem::update_time_step()
	{
		m_fixed_time_step = m_engine->time_step_fixed();
		m_collision_steps = static_cast<int>(std::ceil(m_fixed_time_step / m_ideal_time_step));
	}

	void JoltPhysicsSubsystem::update_components()
	{
		const auto registry = m_engine->get_subsystem<ecs::EnTTSubsystem>()->registry();

		// Update Spheres
		{
			for (const auto& id : m_spheres_to_init)
			{
				entt::entity entity = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& sphere = registry->get<const SphereComponent3D>(entity);

				init_sphere(id, transform, sphere);
			}

			m_spheres_to_init.clear();
		}

		// Update Boxes
		{
			for (const auto& id : m_boxes_to_init)
			{
				entt::entity entity = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& box = registry->get<const BoxComponent3D>(entity);

				init_box(id, transform, box);
			}

			m_boxes_to_init.clear();
		}

		// Update Rigidbodies
		{
			// Create Bodies
			for (const auto& id : m_bodies_to_init)
			{
				entt::entity entity = m_engine->get_subsystem<ecs::EnTTSubsystem>()->get_entity(id);

				const auto& transform = registry->get<const TransformComponent3D>(entity);
				const auto& rb = registry->get<const RigidbodyComponent3D>(entity);

				if (m_shape_refs.contains(id))
				{
					init_rigidbody(id, transform, rb);
				}
			}

			m_bodies_to_init.clear();

			// Add bodies to physics system
			if (!m_bodies_to_add.empty())
			{
				std::vector<JPH::BodyID> bodyIDs;
				bodyIDs.reserve(m_bodies_to_add.size());

				for (const auto& id : m_bodies_to_add)
				{
					bodyIDs.emplace_back(m_bodies[id]->GetID());
				}

				m_bodies_to_add.clear();

				JPH::BodyInterface& bodyInterface = m_internal_physics_system->GetBodyInterface();

				const JPH::BodyInterface::AddState addState = bodyInterface.AddBodiesPrepare(bodyIDs.data(), bodyIDs.size());
				bodyInterface.AddBodiesFinalize(bodyIDs.data(), bodyIDs.size(), addState, JPH::EActivation::Activate);
			}
		}
	}

	void JoltPhysicsSubsystem::init_box(PuffinID id, const TransformComponent3D& transform, const BoxComponent3D& box)
	{
		if (m_internal_physics_system)
		{
			JPH::BoxShapeSettings boxShapeSettings(JPH::Vec3(box.half_extent.x, box.half_extent.y, box.half_extent.z));

			JPH::ShapeSettings::ShapeResult result = boxShapeSettings.Create();

			if (result.HasError())
			{
				std::cout << "Failed to create box shape, id: " << id << " error: " << result.GetError() << std::endl;
				return;
			}

			m_shape_refs.emplace(id, result.Get());
		}
	}

	void JoltPhysicsSubsystem::init_sphere(PuffinID id, const TransformComponent3D& transform,
		const SphereComponent3D& circle)
	{
		if (m_internal_physics_system)
		{
			// PUFFIN_TODO - Implement Sphere Creation
		}
	}

	void JoltPhysicsSubsystem::init_rigidbody(PuffinID id, const TransformComponent3D& transform,
		const RigidbodyComponent3D& rb)
	{
		if (m_internal_physics_system && m_shape_refs.contains(id))
		{
			JPH::BodyInterface& bodyInterface = m_internal_physics_system->GetBodyInterface();

			const JPH::EMotionType motionType = g_jolt_body_type.at(rb.body_type);

			JPH::ObjectLayer objectLayer = {};

			if (motionType == JPH::EMotionType::Static)
			{
				objectLayer = JoltLayers::NON_MOVING;
			}
			else
			{
				objectLayer = JoltLayers::MOVING;
			}

			JPH::BodyCreationSettings bodySettings(m_shape_refs[id], JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
				JPH::QuatArg::sIdentity(), motionType, objectLayer);

			bodySettings.mRestitution = rb.elasticity;

			JPH::MassProperties massProperties = bodySettings.GetMassProperties();
			massProperties.ScaleToMass(rb.mass);

			bodySettings.mMassPropertiesOverride = massProperties;
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;

			bodySettings.mFriction = 0.0f;

			m_bodies.emplace(id, bodyInterface.CreateBody(bodySettings));

			m_bodies_to_add.push_back(id);
		}
	}
}

#endif // PFN_JOLT_PHYSICS
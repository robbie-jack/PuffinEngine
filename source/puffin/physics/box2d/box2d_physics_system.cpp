#include "puffin/physics/box2d/box2d_physics_system.h"

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/signal_subsystem.h"
#include "puffin/core/settings_manager.h"
#include "puffin/math_helpers.h"

namespace puffin::physics
{
	Box2DPhysicsSystem::Box2DPhysicsSystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		m_name = "Box2DPhysicsSystem";
	}

	void Box2DPhysicsSystem::initialize(core::SubsystemManager* subsystem_manager)
	{
		// Bind entt callbacks
		auto registry = m_engine->get_subsystem<ecs::EnTTSubsystem>()->registry();

		registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent2D>>();
		registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent2D>>();

		registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::on_construct_rigidbody>(this);
		registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::on_update_rigidbody>(this);
		registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::on_destroy_rigidbody>(this);

		registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSystem::on_construct_box>(this);
		registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSystem::on_update_box>(this);
		registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSystem::on_destroy_box>(this);

		registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSystem::on_construct_circle>(this);
		registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSystem::on_update_circle>(this);
		registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSystem::on_destroy_circle>(this);

		// Create world
		auto settings_manager = m_engine->get_subsystem<core::SettingsManager>();
		auto gravity = settings_manager->get<Vector3f>("physics_gravity");

		b2WorldDef world_def = b2DefaultWorldDef();
		world_def.gravity = { gravity.x, gravity.y };

		m_physics_world_id = b2CreateWorld(&world_def);

		m_sub_steps = 4;
	}

	void Box2DPhysicsSystem::deinitialize()
	{
		m_sub_steps = 0;

		b2DestroyWorld(m_physics_world_id);
		m_physics_world_id = b2_nullWorldId;

		//m_contact_listener = nullptr;
	}

	core::SubsystemType Box2DPhysicsSystem::type() const
	{
		return core::SubsystemType::Gameplay;
	}

	void Box2DPhysicsSystem::begin_play()
	{
		create_objects();
	}

	void Box2DPhysicsSystem::end_play()
	{
		destroy_objects();
	}

	void Box2DPhysicsSystem::update(double delta_time)
	{
		destroy_objects();

		create_objects();
	}

	bool Box2DPhysicsSystem::should_update()
	{
		return true;
	}

	void Box2DPhysicsSystem::fixed_update(double fixed_time_step)
	{
		b2World_Step(m_physics_world_id, fixed_time_step, m_sub_steps);
	}

	bool Box2DPhysicsSystem::should_fixed_update()
	{
		return true;
	}

	//void Box2DPhysicsSystem::fixedUpdate()
	//{
	//	// Step Physics World
	//	m_physics_world->Step(mEngine->timeStepFixed(), m_velocity_iterations, m_position_iterations);

	//	// Publish Collision Events
	//	publish_collision_events();

	//	const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

	//	// Updated entity position/rotation from simulation
	//	const auto bodyView = registry->view<const SceneObjectComponent, TransformComponent2D, VelocityComponent2D, const RigidbodyComponent2D>();

	//	for (auto [entity, object, transform, velocity, rb] : bodyView.each())
	//	{
	//		const auto& id = object.id;

	//		// Update Transform from Rigidbody Position
	//		registry->patch<TransformComponent2D>(entity, [&](auto& transform)
	//		{
	//			transform.position.x = mBodies[id]->GetPosition().x;
	//			transform.position.y = mBodies[id]->GetPosition().y;
	//			transform.rotation = maths::radToDeg(-mBodies[id]->GetAngle());
	//		});

	//		// Update Velocity with Linear/Angular Velocity#
	//		registry->patch<VelocityComponent2D>(entity, [&](auto& velocity)
	//		{
	//			velocity.linear.x = mBodies[id]->GetLinearVelocity().x;
	//			velocity.linear.y = mBodies[id]->GetLinearVelocity().y;
	//		});
	//	}
	//}

	void Box2DPhysicsSystem::on_construct_box(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::on_update_box(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::on_destroy_box(entt::registry& registry, entt::entity entity)
	{
		

		
	}

	void Box2DPhysicsSystem::on_construct_circle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::on_update_circle(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::on_destroy_circle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::on_construct_rigidbody(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::on_update_rigidbody(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::on_destroy_rigidbody(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::create_objects()
	{
		const auto entt_subsystem = m_engine->get_subsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		// Create bodies
		//{
		//	for (const auto& id : m_bodies_to_create)
		//	{

		//	}

		//	m_bodies_to_create.clear();
		//}

		//// Create circles
		//{
		//	for (const auto& id : m_circles_to_create)
		//	{

		//	}

		//	m_circles_to_create.clear();
		//}

		//// Create 
		//{
		//	for (const auto& id : m_boxes_to_create)
		//	{

		//	}

		//	m_boxes_to_create.clear();
		//}

		
	}

	void Box2DPhysicsSystem::destroy_objects()
	{

	}

	void Box2DPhysicsSystem::publish_collision_events() const
	{
		const auto signalSubsystem = m_engine->get_subsystem<core::SignalSubsystem>();

		//CollisionBeginEvent collision_begin_event;
		//while (m_contact_listener->getNextCollisionBeginEvent(collision_begin_event))
		//{
		//	//signalSubsystem->signal(collision_begin_event);
		//}

		//CollisionEndEvent collision_end_event;
		//while (m_contact_listener->getNextCollisionEndEvent(collision_end_event))
		//{
		//	//signalSubsystem->signal(collision_end_event);
		//}
	}

	void Box2DPhysicsSystem::create_body(PuffinID id, const TransformComponent2D& transform, const RigidbodyComponent2D& rb)
	{
		
	}

	void Box2DPhysicsSystem::create_box(PuffinID id, const TransformComponent2D& transform, const BoxComponent2D& box)
	{
		
	}

	void Box2DPhysicsSystem::create_circle(PuffinID id, const TransformComponent2D& transform, const CircleComponent2D& circle)
	{
		
	}

	void Box2DPhysicsSystem::update_body(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::update_box(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::update_circle(PuffinID id)
	{

	}

	void Box2DPhysicsSystem::destroy_body(PuffinID id)
	{
		
	}

	void Box2DPhysicsSystem::destroy_box(PuffinID id)
	{
		
	}

	void Box2DPhysicsSystem::destroy_circle(PuffinID id)
	{
		
	}
}

#endif // PFN_BOX2D_PHYSICS
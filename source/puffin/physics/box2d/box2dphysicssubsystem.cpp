#include "puffin/physics/box2d/box2dphysicssubsystem.h"

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/mathhelpers.h"

#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"

namespace puffin::physics
{
	Box2DPhysicsSystem::Box2DPhysicsSystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		mName = "Box2DPhysicsSystem";
	}

	void Box2DPhysicsSystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();
		
		// Bind entt callbacks
		auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		registry->on_construct<RigidbodyComponent2D>().connect<&entt::registry::emplace<VelocityComponent2D>>();
		registry->on_destroy<RigidbodyComponent2D>().connect<&entt::registry::remove<VelocityComponent2D>>();

		registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnConstructRigidbody>(this);
		registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnUpdateRigidbody>(this);
		registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyRigidbody>(this);

		registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnConstructBox>(this);
		registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnUpdateBox>(this);
		registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyBox>(this);

		registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnConstructCircle>(this);
		registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnUpdateCircle>(this);
		registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSystem::OnDestroyCircle>(this);

		InitSettingsAndSignals();
		
		// Create world
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = mGravity;
		
		mPhysicsWorldID = b2CreateWorld(&worldDef);
	}

	void Box2DPhysicsSystem::Deinitialize()
	{
		mGravity = { 0.0, 0.0 };
		mSubSteps = 0;

		b2DestroyWorld(mPhysicsWorldID);
		mPhysicsWorldID = b2_nullWorldId;

		//m_contact_listener = nullptr;
	}

	core::SubsystemType Box2DPhysicsSystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void Box2DPhysicsSystem::BeginPlay()
	{
		CreateObjects();
	}

	void Box2DPhysicsSystem::EndPlay()
	{
		DestroyObjects();
	}

	void Box2DPhysicsSystem::Update(double deltaTime)
	{
		DestroyObjects();

		CreateObjects();
	}

	bool Box2DPhysicsSystem::ShouldUpdate()
	{
		return mEnabled;
	}

	void Box2DPhysicsSystem::FixedUpdate(double fixedTimeStep)
	{
		b2World_Step(mPhysicsWorldID, fixedTimeStep, mSubSteps);
	}

	bool Box2DPhysicsSystem::ShouldFixedUpdate()
	{
		return mEnabled;
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

	void Box2DPhysicsSystem::OnConstructBox(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::OnUpdateBox(entt::registry& registry, entt::entity entity)
	{

	}

	void Box2DPhysicsSystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
	{

	}

	void Box2DPhysicsSystem::OnConstructCircle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::OnUpdateCircle(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::OnDestroyCircle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::OnConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::OnUpdateRigidbody(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSystem::InitSettingsAndSignals()
	{
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
		
		auto gravityX = settingsManager->Get<float>("physics", "gravity_x").value_or(0.0);
		auto gravityY = settingsManager->Get<float>("physics", "gravity_y").value_or(-9.81);

		mGravity = { gravityX, gravityY };

		auto gravityXSignal = signalSubsystem->GetOrCreateSignal("physics_gravity_x");
		gravityXSignal->Connect(std::function([&]
		{
			auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

			mGravity.x = settingsManager->Get<float>("physics", "gravity_x").value_or(0.0);

			if (b2World_IsValid(mPhysicsWorldID))
			{
				b2World_SetGravity(mPhysicsWorldID, mGravity);
			}
		}));

		auto gravityYSignal = signalSubsystem->GetOrCreateSignal("physics_gravity_y");
		gravityYSignal->Connect(std::function([&]
		{
			auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

			mGravity.y = settingsManager->Get<float>("physics", "gravity_y").value_or(-9.81);

			if (b2World_IsValid(mPhysicsWorldID))
			{
				b2World_SetGravity(mPhysicsWorldID, mGravity);
			}
		}));

		mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);

		auto subStepsSignal = signalSubsystem->GetOrCreateSignal("physics_sub_steps");
		subStepsSignal->Connect(std::function([&]
		{
			auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

			mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);
		}));

		mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);

		auto box2dEnableSignal = signalSubsystem->GetOrCreateSignal("physics_box2d_enable");
		box2dEnableSignal->Connect(std::function([&]
		{
			mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);
		}));
	}

	void Box2DPhysicsSystem::CreateObjects()
	{
		const auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->GetRegistry();

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

	void Box2DPhysicsSystem::DestroyObjects()
	{

	}

	void Box2DPhysicsSystem::PublishCollisionEvents() const
	{
		const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

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

	void Box2DPhysicsSystem::CreateBody(UUID id, const TransformComponent2D& transform, const RigidbodyComponent2D& rb)
	{
		
	}

	void Box2DPhysicsSystem::CreateBox(UUID id, const TransformComponent2D& transform, const BoxComponent2D& box)
	{
		
	}

	void Box2DPhysicsSystem::CreateCircle(UUID id, const TransformComponent2D& transform, const CircleComponent2D& circle)
	{
		
	}

	void Box2DPhysicsSystem::UpdateBody(UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateBox(UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateCircle(UUID id)
	{

	}

	void Box2DPhysicsSystem::DestroyBody(UUID id)
	{
		
	}

	void Box2DPhysicsSystem::DestroyBox(UUID id)
	{
		
	}

	void Box2DPhysicsSystem::DestroyCircle(UUID id)
	{
		
	}
}

#endif // PFN_BOX2D_PHYSICS
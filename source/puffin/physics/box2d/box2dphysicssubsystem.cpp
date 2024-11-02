#include "puffin/physics/box2d/box2dphysicssubsystem.h"

#if PFN_BOX2D_PHYSICS

#include "puffin/core/engine.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/mathhelpers.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/physics/onager2d/colliders/boxcollider2d.h"

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
	}

	void Box2DPhysicsSystem::Deinitialize()
	{
		mGravity = { 0.0, 0.0 };
		mSubSteps = 0;

		//m_contact_listener = nullptr;
	}

	core::SubsystemType Box2DPhysicsSystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void Box2DPhysicsSystem::BeginPlay()
	{
		// Create world
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = mGravity;
		
		mPhysicsWorldID = b2CreateWorld(&worldDef);

		const auto& registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		
		const auto& bodyView = registry->view<RigidbodyComponent2D>();
		for (auto& [entity, rb] : bodyView.each())
		{
			OnConstructRigidbody(*registry, entity);
		}

		const auto& boxView = registry->view<BoxComponent2D>();
		for (auto& [entity, box] : boxView.each())
		{
			OnConstructBox(*registry, entity);
		}
		
		CreateObjects();
	}

	void Box2DPhysicsSystem::EndPlay()
	{
		DestroyObjects();

		b2DestroyWorld(mPhysicsWorldID);
		mPhysicsWorldID = b2_nullWorldId;
	}

	void Box2DPhysicsSystem::FixedUpdate(double fixedTimeStep)
	{
		DestroyObjects();
		CreateObjects();
		
		b2World_Step(mPhysicsWorldID, fixedTimeStep, mSubSteps);

		const auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		const auto& view = registry->view<RigidbodyComponent2D>();

		for (auto& [entity, rb] : view.each())
		{
			
		}
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
		if (!registry.valid(entity))
		{
			return;
		}
		
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);
		
		mShapeCreateEvents.Push({ entity, id, ShapeType2D::Box });
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
		if (!registry.valid(entity))
		{
			return;
		}
		
		if (registry.any_of<TransformComponent2D>(entity) && !registry.any_of<VelocityComponent2D>(entity))
		{
			registry.emplace<VelocityComponent2D>(entity);
		}

		if (registry.any_of<TransformComponent3D>(entity) && !registry.any_of<VelocityComponent3D>(entity))
		{
			registry.emplace<VelocityComponent3D>(entity);
		}

		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);
		
		mBodyCreateEvents.Push({ entity, id });
	}

	void Box2DPhysicsSystem::OnUpdateRigidbody(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<VelocityComponent2D>(entity))
		{
			registry.remove<VelocityComponent2D>(entity);
		}

		if (registry.any_of<VelocityComponent3D>(entity))
		{
			registry.remove<VelocityComponent3D>(entity);
		}
	}

	void Box2DPhysicsSystem::InitSettingsAndSignals()
	{
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

		// Gravity
		{
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
		}

		// Sub Steps
		{
			mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);

			auto subStepsSignal = signalSubsystem->GetOrCreateSignal("physics_sub_steps");
			subStepsSignal->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);
			}));
		}

		// Enabled
		{
			mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);

			auto box2dEnableSignal = signalSubsystem->GetOrCreateSignal("physics_box2d_enable");
			box2dEnableSignal->Connect(std::function([&]
			{
				mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);
			}));
		}
	}

	void Box2DPhysicsSystem::CreateObjects()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		// Create bodies
		{
			BodyCreateEvent bodyCreateEvent;

			while (mBodyCreateEvents.Pop(bodyCreateEvent))
			{
				CreateBody(bodyCreateEvent.entity, bodyCreateEvent.id);
			}
		}

		// Create Shapes
		{
			ShapeCreateEvent shapeCreateEvent;

			while (mShapeCreateEvents.Pop(shapeCreateEvent))
			{
				switch (shapeCreateEvent.shapeType)
				{
				case ShapeType2D::Box:
					CreateBox(shapeCreateEvent.entity, shapeCreateEvent.id);
					break;
				case ShapeType2D::Circle:
					CreateCircle(shapeCreateEvent.entity, shapeCreateEvent.id);
					break;
				}
			}
		}
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

	void Box2DPhysicsSystem::CreateBody(entt::entity entity, UUID id)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (!registry->valid(entity) || !registry->any_of<RigidbodyComponent2D>(entity))
		{
			return;
		}
		
		const auto& rb = registry->get<RigidbodyComponent2D>(entity);
		
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = gPuffinToBox2DBodyType.at(rb.bodyType);

		if (registry->any_of<TransformComponent2D>(entity))
		{
			const auto& transform = registry->get<TransformComponent2D>(entity);

			bodyDef.position = b2Vec2(transform.position);
			bodyDef.rotation = b2MakeRot(maths::DegToRad(transform.rotation));
		}

		if (registry->any_of<TransformComponent3D>(entity))
		{
			const auto& transform = registry->get<TransformComponent3D>(entity);

			bodyDef.position = {transform.position.x, transform.position.y};
			bodyDef.rotation = b2MakeRot(maths::DegToRad(transform.orientationEulerAngles.roll));
		}

		mBodyIDs.emplace(id, b2CreateBody(mPhysicsWorldID, &bodyDef));
	}

	void Box2DPhysicsSystem::CreateBox(entt::entity entity, UUID id)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (!registry->valid(entity))
		{
			return;
		}

		if (registry->any_of<RigidbodyComponent2D>(entity))
		{
			const auto& rb = registry->get<RigidbodyComponent2D>(entity);
			const auto& box = registry->get<BoxComponent2D>(entity);
		
			auto polygon = b2MakeBox(box.halfExtent.x, box.halfExtent.y);

			b2ShapeDef shapeDef = b2DefaultShapeDef();
			shapeDef.density = rb.density;
			shapeDef.restitution = rb.elasticity;

			mShapeIDs.emplace(id, b2CreatePolygonShape(mBodyIDs.at(id), &shapeDef, &polygon));
			mShapeTypes.emplace(id, ShapeType2D::Box);
		}
	}

	void Box2DPhysicsSystem::CreateCircle(entt::entity entity, UUID id)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (!registry->valid(entity))
		{
			return;
		}

		if (registry->any_of<RigidbodyComponent2D>(entity))
		{
			const auto& rb = registry->get<RigidbodyComponent2D>(entity);
		}
	}

	void Box2DPhysicsSystem::UpdateBody(entt::entity entity, UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateBox(entt::entity entity, UUID id)
	{

	}

	void Box2DPhysicsSystem::UpdateCircle(entt::entity entity, UUID id)
	{

	}

	void Box2DPhysicsSystem::DestroyBody(entt::entity entity, UUID id)
	{
		
	}

	void Box2DPhysicsSystem::DestroyBox(entt::entity entity, UUID id)
	{
		
	}

	void Box2DPhysicsSystem::DestroyCircle(entt::entity entity, UUID id)
	{
		
	}
}

#endif // PFN_BOX2D_PHYSICS
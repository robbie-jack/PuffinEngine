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
#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/core/timer.h"

namespace puffin::physics
{
	static void* EnqueueTask(b2TaskCallback* task, int32_t itemCount, int32_t minRange, void* taskContext, void* userContext)
	{
		Box2DPhysicsSubsystem* box2DSubsystem = static_cast<Box2DPhysicsSubsystem*>(userContext);

		if (box2DSubsystem->TaskCount() < gMaxTasks)
		{
			Box2DTask& box2DTask = box2DSubsystem->Tasks()[box2DSubsystem->TaskCount()];
			box2DTask.m_SetSize = itemCount;
			box2DTask.m_MinRange = minRange;
			box2DTask.mTask = task;
			box2DTask.mTaskContext = taskContext;
			box2DSubsystem->TaskScheduler().AddTaskSetToPipe(&box2DTask);
			++box2DSubsystem->TaskCount();
			return &box2DTask;
		}
		else
		{
			assert(false);
			task(0, itemCount, 0, taskContext);
			return nullptr;
		}
	}

	static void FinishTask(void* taskPtr, void* userContext)
	{
		if (taskPtr != nullptr)
		{
			Box2DTask* box2DTask = static_cast<Box2DTask*>(taskPtr);
			Box2DPhysicsSubsystem* box2DSubsystem = static_cast<Box2DPhysicsSubsystem*>(userContext);
			box2DSubsystem->TaskScheduler().WaitforTask(box2DTask);	
		}
	}
	
	Box2DPhysicsSubsystem::Box2DPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
		mName = "Box2DPhysicsSystem";
	}

	void Box2DPhysicsSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();
		
		// Bind entt callbacks
		auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		mConnections.push_back(registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructRigidbody>(this));
		mConnections.push_back(registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateRigidbody>(this));
		mConnections.push_back(registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyRigidbody>(this));

		mConnections.push_back(registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructBox>(this));
		mConnections.push_back(registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateBox>(this));
		mConnections.push_back(registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyBox>(this));

		mConnections.push_back(registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructCircle>(this));
		mConnections.push_back(registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateCircle>(this));
		mConnections.push_back(registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyCircle>(this));
		
		InitSettingsAndSignals();
	}

	void Box2DPhysicsSubsystem::Deinitialize()
	{
		mGravity = { 0.0, 0.0 };
		mSubSteps = 0;

		auto registry = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		for (auto& connection : mConnections)
		{
			connection.release();
		}

		mConnections.clear();

		//m_contact_listener = nullptr;
	}

	core::SubsystemType Box2DPhysicsSubsystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void Box2DPhysicsSubsystem::BeginPlay()
	{
		auto enkiTSSubsystem = mEngine->GetSubsystem<core::EnkiTSSubsystem>();
		
		// Create world
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = mGravity;
		worldDef.workerCount = enkiTSSubsystem->GetThreadCount();
		worldDef.enqueueTask = EnqueueTask;
		worldDef.finishTask = FinishTask;
		worldDef.userTaskContext = this;
		worldDef.enableSleep = true;

		mTaskCount = 0;
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

	void Box2DPhysicsSubsystem::EndPlay()
	{
		DestroyObjects();

		b2DestroyWorld(mPhysicsWorldID);
		mPhysicsWorldID = b2_nullWorldId;
	}

	void Box2DPhysicsSubsystem::FixedUpdate(double fixedTimeStep)
	{
		DestroyObjects();
		CreateObjects();
		
		b2World_Step(mPhysicsWorldID, fixedTimeStep, mSubSteps);
		mTaskCount = 0;

		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();
		
		const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		const auto& view = registry->view<RigidbodyComponent2D>();

		for (auto& [entity, rb] : view.each())
		{
			const auto id = enttSubsystem->GetID(entity);
			auto* node = sceneGraph->GetNode(id);

			if (mBodyData.find(id) == mBodyData.end())
			{
				continue;
			}
			
			const auto& bodyData = mBodyData.at(id);
			
			b2Vec2 pos = b2Body_GetPosition(bodyData.bodyID);
			b2Vec2 vel = b2Body_GetLinearVelocity(bodyData.bodyID);

			if (registry->all_of<TransformComponent2D, VelocityComponent2D>(entity))
			{
				if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node); transformNode2D)
				{
					transformNode2D->Transform().position.x = pos.x;
					transformNode2D->Transform().position.y = pos.y;
				}
				else
				{
					registry->patch<TransformComponent2D>(entity, [&](auto& transform)
					{
						transform.position.x = pos.x;
						transform.position.y = pos.y;
					});
				}

				registry->patch<VelocityComponent2D>(entity, [&](auto& velocity)
				{
					velocity.linear.x = vel.x;
					velocity.linear.y = vel.y;
				});
			}

			if (registry->all_of<TransformComponent3D, VelocityComponent3D>(entity))
			{
				if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
				{
					transformNode3D->Transform().position.x = pos.x;
					transformNode3D->Transform().position.y = pos.y;
				}
				else
				{
					registry->patch<TransformComponent3D>(entity, [&](auto& transform)
					{
						transform.position.x = pos.x;
						transform.position.y = pos.y;
					});
				}

				registry->patch<VelocityComponent3D>(entity, [&](auto& velocity)
				{
					velocity.linear.x = vel.x;
					velocity.linear.y = vel.y;
				});
			}
		}
	}

	bool Box2DPhysicsSubsystem::ShouldFixedUpdate()
	{
		return mEnabled;
	}

	enki::TaskScheduler& Box2DPhysicsSubsystem::TaskScheduler()
	{
		return *mEngine->GetSubsystem<core::EnkiTSSubsystem>()->GetTaskScheduler();
	}

	void Box2DPhysicsSubsystem::OnConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);
		
		mShapeCreateEvents.Push({ entity, id, ShapeType2D::Box });
	}

	void Box2DPhysicsSubsystem::OnUpdateBox(entt::registry& registry, entt::entity entity)
	{

	}

	void Box2DPhysicsSubsystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
	{
		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mShapeDestroyEvents.Push({ entity, id, ShapeType2D::Box });
	}

	void Box2DPhysicsSubsystem::OnConstructCircle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSubsystem::OnUpdateCircle(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSubsystem::OnDestroyCircle(entt::registry& registry, entt::entity entity)
	{
		
	}

	void Box2DPhysicsSubsystem::OnConstructRigidbody(entt::registry& registry, entt::entity entity)
	{
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

	void Box2DPhysicsSubsystem::OnUpdateRigidbody(entt::registry& registry, entt::entity entity)
	{
	}

	void Box2DPhysicsSubsystem::OnDestroyRigidbody(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<VelocityComponent2D>(entity))
		{
			registry.remove<VelocityComponent2D>(entity);
		}

		if (registry.any_of<VelocityComponent3D>(entity))
		{
			registry.remove<VelocityComponent3D>(entity);
		}

		const auto id = mEngine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mBodyDestroyEvents.Push({ entity, id });
	}

	void Box2DPhysicsSubsystem::InitSettingsAndSignals()
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

	void Box2DPhysicsSubsystem::CreateObjects()
	{
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
					CreateBox(shapeCreateEvent.entity, shapeCreateEvent.id, shapeCreateEvent.id);
					break;
				case ShapeType2D::Circle:
					CreateCircle(shapeCreateEvent.entity, shapeCreateEvent.id, shapeCreateEvent.id);
					break;
				}
			}
		}
	}

	void Box2DPhysicsSubsystem::DestroyObjects()
	{
		// Destroy Bodies
		{
			BodyDestroyEvent bodyDestroyEvent;

			while (mBodyDestroyEvents.Pop(bodyDestroyEvent))
			{
				DestroyBody(bodyDestroyEvent.entity, bodyDestroyEvent.id);
			}
		}

		// Destroy Shapes
		{
			ShapeDestroyEvent shapeDestroyEvent;

			while (mShapeDestroyEvents.Pop(shapeDestroyEvent))
			{
				switch (shapeDestroyEvent.shapeType)
				{
				case ShapeType2D::Box:
					DestroyBox(shapeDestroyEvent.entity, shapeDestroyEvent.id);
					break;
				case ShapeType2D::Circle:
					DestroyCircle(shapeDestroyEvent.entity, shapeDestroyEvent.id);
					break;
				}
			}
		}
	}

	void Box2DPhysicsSubsystem::PublishCollisionEvents() const
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

	void Box2DPhysicsSubsystem::CreateBody(entt::entity entity, UUID id)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (!registry->valid(entity) || !registry->any_of<RigidbodyComponent2D>(entity))
		{
			return;
		}

		mUserData.emplace(id, UserData{id, entity});
		
		const auto& rb = registry->get<RigidbodyComponent2D>(entity);
		
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = gPuffinToBox2DBodyType.at(rb.bodyType);
		bodyDef.userData = &mUserData.at(id);

		if (registry->all_of<TransformComponent2D, VelocityComponent2D>(entity))
		{
			const auto& transform = registry->get<TransformComponent2D>(entity);
			const auto& velocity = registry->get<VelocityComponent2D>(entity);

			bodyDef.position = b2Vec2(transform.position);
			bodyDef.rotation = b2MakeRot(maths::DegToRad(transform.rotation));
			bodyDef.linearVelocity = b2Vec2(velocity.linear);
		}

		if (registry->all_of<TransformComponent3D, VelocityComponent3D>(entity))
		{
			const auto& transform = registry->get<TransformComponent3D>(entity);
			const auto& velocity = registry->get<VelocityComponent3D>(entity);

			bodyDef.position = { transform.position.x, transform.position.y };
			bodyDef.rotation = b2MakeRot(maths::DegToRad(transform.orientationEulerAngles.roll));
			bodyDef.linearVelocity = { velocity.linear.x, velocity.linear.y };
		}

		b2BodyId bodyID = b2CreateBody(mPhysicsWorldID, &bodyDef);
		mBodyData.emplace(id, BodyData{bodyID});
	}

	void Box2DPhysicsSubsystem::CreateBox(entt::entity entity, UUID boxUUID, UUID bodyUUID)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (!registry->valid(entity))
		{
			return;
		}

		if (registry->any_of<RigidbodyComponent2D>(entity))
		{
			if (mUserData.find(boxUUID) == mUserData.end())
			{
				mUserData.emplace(boxUUID, UserData{boxUUID, entity});
			}
			
			const auto& rb = registry->get<RigidbodyComponent2D>(entity);
			const auto& box = registry->get<BoxComponent2D>(entity);
		
			auto polygon = b2MakeBox(box.halfExtent.x, box.halfExtent.y);

			b2ShapeDef shapeDef = b2DefaultShapeDef();
			shapeDef.density = rb.density;
			shapeDef.restitution = rb.elasticity;
			shapeDef.friction = rb.friction;
			shapeDef.userData = &mUserData.at(boxUUID);

			b2ShapeId shapeID = b2CreatePolygonShape(mBodyData.at(bodyUUID).bodyID, &shapeDef, &polygon);
			mShapeData.emplace(boxUUID, ShapeData{shapeID, ShapeType2D::Box});
			mBodyData.at(bodyUUID).shapeIDs.emplace(boxUUID);
		}
	}

	void Box2DPhysicsSubsystem::CreateCircle(entt::entity entity, UUID circleUUID, UUID bodyUUID)
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

	void Box2DPhysicsSubsystem::UpdateBody(entt::entity entity, UUID id)
	{
		
	}

	void Box2DPhysicsSubsystem::UpdateBox(entt::entity entity, UUID id)
	{

	}

	void Box2DPhysicsSubsystem::UpdateCircle(entt::entity entity, UUID id)
	{

	}

	void Box2DPhysicsSubsystem::DestroyBody(entt::entity entity, UUID id)
	{
		if (b2Body_IsValid(mBodyData.at(id).bodyID))
		{
			b2DestroyBody(mBodyData.at(id).bodyID);
		}

		mUserData.erase(id);
		mBodyData.erase(id);
	}

	void Box2DPhysicsSubsystem::DestroyBox(entt::entity entity, UUID id)
	{
		if (b2Shape_IsValid(mShapeData.at(id).shapeID))
		{
			b2BodyId bodyID = b2Shape_GetBody(mShapeData.at(id).shapeID);

			auto* userData = static_cast<UserData*>(b2Body_GetUserData(bodyID));

			BodyData& bodyData = mBodyData.at(userData->id);
			bodyData.shapeIDs.erase(id);
			
			b2DestroyShape(mShapeData.at(id).shapeID, true);
		}

		mUserData.erase(id);
		mShapeData.erase(id);
	}

	void Box2DPhysicsSubsystem::DestroyCircle(entt::entity entity, UUID id)
	{
		
	}
}

#endif // PFN_BOX2D_PHYSICS
#include "physics/box2d/box2d_physics_subsystem.h"

#if PFN_BOX2D_PHYSICS

#include "core/engine.h"
#include "core/signal_subsystem.h"
#include "core/settings_manager.h"
#include "math_helpers.h"
#include "component/transform_component_2d.h"
#include "component/transform_component_3d.h"
#include "component/physics/2d/box_component_2d.h"
#include "component/physics/2d/circle_component_2d.h"
#include "component/physics/2d/rigidbody_component_2d.h"
#include "component/physics/2d/velocity_component_2d.h"
#include "component/physics/3d/velocity_component_3d.h"
#include "physics/onager2d/colliders/box_collider_2d.h"
#include "node/transform_2d_node.h"
#include "node/transform_3d_node.h"
#include "node/physics/2d/rigidbody_2d_node.h"
#include "node/physics/2d/box_2d_node.h"
#include "scene/scene_graph_subsystem.h"
#include "core/enkits_subsystem.h"
#include "core/timer.h"

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
	
	Box2DPhysicsSubsystem::Box2DPhysicsSubsystem(const std::shared_ptr<core::Engine>& engine): GameplaySubsystem(engine)
	{
	}

	void Box2DPhysicsSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		GameplaySubsystem::Initialize(subsystemManager);

		subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

		InitConnections();
		InitSettingsAndSignals();
	}

	void Box2DPhysicsSubsystem::Deinitialize()
	{
		GameplaySubsystem::Deinitialize();

		mGravity = { 0.0, 0.0 };
		mSubSteps = 0;

		auto registry = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		for (auto& connection : mConnections)
		{
			connection.release();
		}

		mConnections.clear();

		//m_contact_listener = nullptr;
	}

	void Box2DPhysicsSubsystem::BeginPlay()
	{
		auto enkiTSSubsystem = m_engine->GetSubsystem<core::EnkiTSSubsystem>();
		
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

		const auto* sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		std::vector<Rigidbody2DNode*> bodies;
		sceneGraph->GetNodes(bodies);
		for (auto* body : bodies)
		{
			mBodyCreateEvents.Push({ body->GetID() });
		}

		std::vector<Box2DNode*> boxes;
		sceneGraph->GetNodes(boxes);
		for (auto* box : boxes)
		{
			mShapeCreateEvents.Push({ box->GetID(), ShapeType2D::Box });
		}

		const auto& registry = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		
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

		UpdateRigidbodyNodes();
		UpdateRigidbodyComponents();
	}

	bool Box2DPhysicsSubsystem::ShouldFixedUpdate()
	{
		return mEnabled;
	}

	std::string_view Box2DPhysicsSubsystem::GetName() const
	{
		return reflection::GetTypeString<Box2DPhysicsSubsystem>();
	}

	enki::TaskScheduler& Box2DPhysicsSubsystem::TaskScheduler()
	{
		return *m_engine->GetSubsystem<core::EnkiTSSubsystem>()->GetTaskScheduler();
	}

	void Box2DPhysicsSubsystem::OnConstructBox(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);
		
		mShapeCreateEvents.Push({ id, ShapeType2D::Box });
	}

	void Box2DPhysicsSubsystem::OnUpdateBox(entt::registry& registry, entt::entity entity)
	{

	}

	void Box2DPhysicsSubsystem::OnDestroyBox(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mShapeDestroyEvents.Push({ id, ShapeType2D::Box });
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

		const auto id = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);
		
		mBodyCreateEvents.Push({ id });
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

		const auto id = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetID(entity);

		mBodyDestroyEvents.Push({ id });
	}

	void Box2DPhysicsSubsystem::InitConnections()
	{
		// Bind entt callbacks
		auto registry = m_engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();

		mConnections.push_back(registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructRigidbody>(this));
		mConnections.push_back(registry->on_construct<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateRigidbody>(this));
		mConnections.push_back(registry->on_destroy<RigidbodyComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyRigidbody>(this));

		mConnections.push_back(registry->on_construct<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructBox>(this));
		mConnections.push_back(registry->on_update<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateBox>(this));
		mConnections.push_back(registry->on_destroy<BoxComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyBox>(this));

		mConnections.push_back(registry->on_construct<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnConstructCircle>(this));
		mConnections.push_back(registry->on_update<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnUpdateCircle>(this));
		mConnections.push_back(registry->on_destroy<CircleComponent2D>().connect<&Box2DPhysicsSubsystem::OnDestroyCircle>(this));
	}

	void Box2DPhysicsSubsystem::InitSettingsAndSignals()
	{
		auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
		auto signalSubsystem = m_engine->GetSubsystem<core::SignalSubsystem>();

		// Gravity
		auto gravityX = settingsManager->Get<float>("physics", "gravity_x").value_or(0.0);
		auto gravityY = settingsManager->Get<float>("physics", "gravity_y").value_or(-9.81);

		mGravity = { gravityX, gravityY };

		auto gravityXSignal = signalSubsystem->GetOrCreateSignal("physics_gravity_x");
		gravityXSignal->Connect(std::function([&]
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mGravity.x = settingsManager->Get<float>("physics", "gravity_x").value_or(0.0);

			if (b2World_IsValid(mPhysicsWorldID))
			{
				b2World_SetGravity(mPhysicsWorldID, mGravity);
			}
		}));

		auto gravityYSignal = signalSubsystem->GetOrCreateSignal("physics_gravity_y");
		gravityYSignal->Connect(std::function([&]
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mGravity.y = settingsManager->Get<float>("physics", "gravity_y").value_or(-9.81);

			if (b2World_IsValid(mPhysicsWorldID))
			{
				b2World_SetGravity(mPhysicsWorldID, mGravity);
			}
		}));

		// Sub Steps
		mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);

		auto subStepsSignal = signalSubsystem->GetOrCreateSignal("physics_sub_steps");
		subStepsSignal->Connect(std::function([&]
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mSubSteps = settingsManager->Get<int>("physics", "sub_steps").value_or(4);
		}));

		// Enabled
		mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);

		auto box2dEnableSignal = signalSubsystem->GetOrCreateSignal("physics_box2d_enable");
		box2dEnableSignal->Connect(std::function([&]
		{
			mEnabled = settingsManager->Get<bool>("physics", "box2d_enable").value_or(true);
		}));
	}

	void Box2DPhysicsSubsystem::CreateObjects()
	{
		const auto* sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();
		const auto* enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();

		// Create bodies
		BodyCreateEvent bodyCreateEvent;

		while (mBodyCreateEvents.Pop(bodyCreateEvent))
		{
			if (sceneGraph->IsValidNode(bodyCreateEvent.id))
			{
				CreateBodyNode(bodyCreateEvent.id);
			}
			else if (enttSubsystem->IsEntityValid(bodyCreateEvent.id))
			{
				CreateBodyComponent(bodyCreateEvent.id);
			}
		}

		// Create Shapes
		ShapeCreateEvent shapeCreateEvent;

		while (mShapeCreateEvents.Pop(shapeCreateEvent))
		{
			auto* node = sceneGraph->GetNode(shapeCreateEvent.id);

			switch (shapeCreateEvent.shapeType)
			{
			case ShapeType2D::Box:

				if (sceneGraph->IsValidNode(shapeCreateEvent.id))
				{
					CreateBoxNode(shapeCreateEvent.id, node->GetParentID());
				}
				else if (enttSubsystem->IsEntityValid(shapeCreateEvent.id))
				{
					CreateBoxComponent(shapeCreateEvent.id, shapeCreateEvent.id);
				}

				break;

			case ShapeType2D::Circle:

				if (sceneGraph->IsValidNode(shapeCreateEvent.id))
				{
					CreateCircleNode(shapeCreateEvent.id, node->GetParentID());
				}
				else if (enttSubsystem->IsEntityValid(shapeCreateEvent.id))
				{
					CreateCircleComponent(shapeCreateEvent.id, shapeCreateEvent.id);
				}

				break;
			}
		}
	}

	void Box2DPhysicsSubsystem::DestroyObjects()
	{
		// Destroy Bodies
		BodyDestroyEvent bodyDestroyEvent;

		while (mBodyDestroyEvents.Pop(bodyDestroyEvent))
		{
			DestroyBody(bodyDestroyEvent.id);
		}

		// Destroy Shapes
		ShapeDestroyEvent shapeDestroyEvent;

		while (mShapeDestroyEvents.Pop(shapeDestroyEvent))
		{
			switch (shapeDestroyEvent.shapeType)
			{
			case ShapeType2D::Box:
				DestroyBox(shapeDestroyEvent.id);
				break;
			case ShapeType2D::Circle:
				DestroyCircle(shapeDestroyEvent.id);
				break;
			}
		}
	}

	void Box2DPhysicsSubsystem::PublishCollisionEvents() const
	{
		const auto signalSubsystem = m_engine->GetSubsystem<core::SignalSubsystem>();

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

	void Box2DPhysicsSubsystem::UpdateRigidbodyNodes()
	{
		const auto sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		std::vector<Rigidbody2DNode*> bodies;
		sceneGraph->GetNodes(bodies);
		for (auto& body : bodies)
		{
			const auto id = body->GetID();

			if (mBodyData.find(id) == mBodyData.end())
				continue;

			const auto& bodyData = mBodyData.at(id);

			b2Vec2 pos = b2Body_GetPosition(bodyData.bodyID);
			b2Vec2 vel = b2Body_GetLinearVelocity(bodyData.bodyID);

			body->SetGlobalPosition({ pos.x, pos.y });
			body->SetLinearVelocity({ vel.x, vel.y });
		}
	}

	void Box2DPhysicsSubsystem::UpdateRigidbodyComponents()
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto& view = registry->view<RigidbodyComponent2D>();

		for (auto& [entity, rb] : view.each())
		{
			const auto id = enttSubsystem->GetID(entity);

			if (mBodyData.find(id) == mBodyData.end())
				continue;

			const auto& bodyData = mBodyData.at(id);

			b2Vec2 pos = b2Body_GetPosition(bodyData.bodyID);
			b2Vec2 vel = b2Body_GetLinearVelocity(bodyData.bodyID);

			if (registry->all_of<TransformComponent2D, VelocityComponent2D>(entity))
			{
				registry->patch<TransformComponent2D>(entity, [&](auto& transform)
				{
					transform.position.x = pos.x;
					transform.position.y = pos.y;
				});

				registry->patch<VelocityComponent2D>(entity, [&](auto& velocity)
				{
					velocity.linear.x = vel.x;
					velocity.linear.y = vel.y;
				});
			}

			if (registry->all_of<TransformComponent3D, VelocityComponent3D>(entity))
			{
				registry->patch<TransformComponent3D>(entity, [&](auto& transform)
				{
					transform.position.x = pos.x;
					transform.position.y = pos.y;
				});

				registry->patch<VelocityComponent3D>(entity, [&](auto& velocity)
				{
					velocity.linear.x = vel.x;
					velocity.linear.y = vel.y;
				});
			}
		}
	}

	void Box2DPhysicsSubsystem::CreateBodyComponent(UUID id)
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		auto entity = enttSubsystem->GetEntity(id);

		if (!registry->valid(entity) || !registry->any_of<RigidbodyComponent2D>(entity))
			return;

		mUserData.emplace(id, UserData{id});
		
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
		mBodyData.emplace(id, BodyData{bodyID, {}});
	}

	void Box2DPhysicsSubsystem::CreateBoxComponent(UUID boxId, UUID bodyId)
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		auto entity = enttSubsystem->GetEntity(boxId);

		if (!registry->valid(entity))
			return;

		if (!registry->any_of<RigidbodyComponent2D>(entity))
			return;

		if (mUserData.find(boxId) == mUserData.end())
			mUserData.emplace(boxId, UserData{boxId});
		
		const auto& rb = registry->get<RigidbodyComponent2D>(entity);
		const auto& box = registry->get<BoxComponent2D>(entity);
	
		auto polygon = b2MakeBox(box.halfExtent.x, box.halfExtent.y);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = rb.density;
		shapeDef.restitution = rb.elasticity;
		shapeDef.friction = rb.friction;
		shapeDef.userData = &mUserData.at(boxId);

		b2ShapeId shapeID = b2CreatePolygonShape(mBodyData.at(bodyId).bodyID, &shapeDef, &polygon);
		mShapeData.emplace(boxId, ShapeData{shapeID, ShapeType2D::Box});
		mBodyData.at(bodyId).shapeIDs.emplace(boxId);
	}

	void Box2DPhysicsSubsystem::CreateCircleComponent(UUID circleId, UUID bodyId)
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		auto entity = enttSubsystem->GetEntity(circleId);

		if (!registry->valid(entity))
			return;

		if (!registry->any_of<RigidbodyComponent2D>(entity))
			return;

		const auto& rb = registry->get<RigidbodyComponent2D>(entity);
	}

	void Box2DPhysicsSubsystem::CreateBodyNode(UUID id)
	{
		const auto* sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		auto* node = sceneGraph->GetNode<Rigidbody2DNode>(id);
		if (!node)
			return;

		mUserData.emplace(id, UserData{ id });

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = gPuffinToBox2DBodyType.at(node->GetBodyType());
		bodyDef.userData = &mUserData.at(id);

		bodyDef.position = b2Vec2(node->GetGlobalPosition());
		bodyDef.rotation = b2MakeRot(maths::DegToRad(node->GetRotation()));
		bodyDef.linearVelocity = b2Vec2(node->GetLinearVelocity());

		b2BodyId bodyId = b2CreateBody(mPhysicsWorldID, &bodyDef);
		mBodyData.emplace(id, BodyData{ bodyId, {} });
	}

	void Box2DPhysicsSubsystem::CreateBoxNode(UUID boxId, UUID bodyId)
	{
		const auto* sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		auto* box = sceneGraph->GetNode<Box2DNode>(boxId);
		if (!box)
			return;

		auto* body = sceneGraph->GetNode<Rigidbody2DNode>(bodyId);
		if (!body)
			return;

		if (mUserData.find(boxId) == mUserData.end())
			mUserData.emplace(boxId, UserData{ boxId });

		auto polygon = b2MakeBox(box->GetHalfExtent().x, box->GetHalfExtent().y);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = body->GetDensity();
		shapeDef.restitution = body->GetElasticity();
		shapeDef.friction = body->GetFriction();
		shapeDef.userData = &mUserData.at(boxId);

		b2ShapeId shapeId = b2CreatePolygonShape(mBodyData.at(bodyId).bodyID, &shapeDef, &polygon);
		mShapeData.emplace(boxId, ShapeData{ shapeId, ShapeType2D::Box });
		mBodyData.at(bodyId).shapeIDs.emplace(boxId);
	}

	void Box2DPhysicsSubsystem::CreateCircleNode(UUID circleId, UUID bodyId)
	{

	}

	void Box2DPhysicsSubsystem::DestroyBody(UUID id)
	{
		if (b2Body_IsValid(mBodyData.at(id).bodyID))
		{
			b2DestroyBody(mBodyData.at(id).bodyID);
		}

		mUserData.erase(id);
		mBodyData.erase(id);
	}

	void Box2DPhysicsSubsystem::DestroyBox(UUID id)
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

	void Box2DPhysicsSubsystem::DestroyCircle(UUID id)
	{
		
	}
}

#endif // PFN_BOX2D_PHYSICS
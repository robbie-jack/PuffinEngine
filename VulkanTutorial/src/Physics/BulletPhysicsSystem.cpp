#include <Physics/BulletPhysicsSystem.h>
#include <Components/TransformComponent.h>

namespace Puffin
{
	namespace Physics
	{
		void BulletPhysicsSystem::Init()
		{
			rigidbodyEvents = std::make_shared<RingBuffer<RigidbodyEvent>>();

			world->RegisterEvent<RigidbodyEvent>();
			world->SubscribeToEvent<RigidbodyEvent>(rigidbodyEvents);
		}

		void BulletPhysicsSystem::Start()
		{
			// Initialise Bullet Physics Objects
			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			broadphaseInterface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver;
			physicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphaseInterface, solver, collisionConfig);

			physicsWorld->setGravity(btVector3(0, -9.81f, 0));

			for (ECS::Entity entity : entityMap.at("Rigidbody"))
			{
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
				RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(entity);
				InitComponent(entity, rigidbody.size, rigidbody.mass, transform.position);
			}
		}

		void BulletPhysicsSystem::Stop()
		{
			for (ECS::Entity entity : entityMap.at("Rigidbody"))
			{
				//CleanupComponent(entity);

				//world->RemoveComponent<RigidbodyComponent>(entity);
			}

			//entityMap.at("Rigidbody").clear();

			for (int j = 0; j < collisionShapes.size(); j++)
			{
				btCollisionShape* shape = collisionShapes[j];
				collisionShapes[j] = 0;
				delete shape;
			}

			delete physicsWorld;

			delete solver;

			delete broadphaseInterface;

			delete dispatcher;

			delete collisionConfig;

			collisionShapes.clear();
		}

		void BulletPhysicsSystem::Update(float dt)
		{
			ProcessEvents();

			// Step Physics Simulation
			physicsWorld->stepSimulation(dt);

			// Update Entity transfrom from Physics Component
			for (ECS::Entity entity : entityMap.at("Rigidbody"))
			{
				TransformComponent& transformComp = world->GetComponent<TransformComponent>(entity);
				RigidbodyComponent& physicsComp = world->GetComponent<RigidbodyComponent>(entity);

				btTransform transform;

				if (physicsComp.body && physicsComp.body->getMotionState())
				{
					physicsComp.body->getMotionState()->getWorldTransform(transform);
				}

				// Update Transform Component Poisiton/Rotation
				transformComp.position = transform.getOrigin();

				// Convert Rotation from Radians to Degrees

				Puffin::Vector3 rotation;
				transform.getRotation().getEulerZYX(rotation.z, rotation.y, rotation.x);

				float PI = 3.14159;
				rotation.x = rotation.x * 180 / PI;
				rotation.y = rotation.y * 180 / PI;
				rotation.z = rotation.z * 180 / PI;

				transformComp.rotation = rotation;
			}
		}

		void BulletPhysicsSystem::InitComponent(ECS::Entity entity, btVector3 size, btScalar mass, btVector3 position)
		{
			// Get Component for this entity, if it exists
			RigidbodyComponent& comp = world->GetComponent<RigidbodyComponent>(entity);

			comp.size = size;
			comp.mass = mass;

			// Construct Box Shape for Component
			comp.shape = new btBoxShape(comp.size);

			// Store shape for potential re-use
			collisionShapes.push_back(comp.shape);

			// Initialise Component with Starting Position
			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(position);

			// Set Body to dynamic if it has any mass
			bool isDynamic = (comp.mass != 0.0f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
				comp.shape->calculateLocalInertia(comp.mass, localInertia);

			// MotionState is used for interpolation and syncing active objects
			btDefaultMotionState* motionState = new btDefaultMotionState(transform);
			
			// Create Rigid Body
			btRigidBody::btRigidBodyConstructionInfo rbInfo(comp.mass, motionState, comp.shape, localInertia);
			comp.body = new btRigidBody(rbInfo);

			physicsWorld->addRigidBody(comp.body);
		}

		void BulletPhysicsSystem::CleanupComponent(ECS::Entity entity)
		{
			RigidbodyComponent& comp = world->GetComponent<RigidbodyComponent>(entity);

			if (comp.body && comp.body->getMotionState())
			{
				delete comp.body->getMotionState();
			}

			physicsWorld->removeRigidBody(comp.body);
			delete comp.body;

			comp.shape = nullptr;
		}

		void BulletPhysicsSystem::ProcessEvents()
		{
			// Process Rigidbody Events
			RigidbodyEvent rigidbodyEvent;
			while (!rigidbodyEvents->IsEmpty())
			{
				if (rigidbodyEvents->Pop(rigidbodyEvent))
				{
					// Get Component for this entity, if it exists
					RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(rigidbodyEvent.entity);
					TransformComponent& transform = world->GetComponent<TransformComponent>(rigidbodyEvent.entity);

					if (rigidbodyEvent.shouldCreate)
					{
						CleanupComponent(rigidbodyEvent.entity);
						InitComponent(rigidbodyEvent.entity, rigidbody.size, rigidbody.mass, transform.position);
					}

					if (rigidbodyEvent.shouldDelete)
					{
						CleanupComponent(rigidbodyEvent.entity);
						world->RemoveComponent<RigidbodyComponent>(rigidbodyEvent.entity);
					}
				}
			}
		}
	}
}
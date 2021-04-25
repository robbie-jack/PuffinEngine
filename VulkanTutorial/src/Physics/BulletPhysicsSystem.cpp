#include <Physics/BulletPhysicsSystem.h>
#include <Components/TransformComponent.h>

namespace Puffin
{
	namespace Physics
	{
		void BulletPhysicsSystem::Init()
		{
			
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
				InitRigidbody(rigidbody, transform);
			}
		}

		void BulletPhysicsSystem::Stop()
		{
			for (ECS::Entity entity : entityMap.at("Rigidbody"))
			{
				CleanupComponent(entity);

				//world->RemoveComponent<RigidbodyComponent>(entity);
			}

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
			for (ECS::Entity entity : entityMap.at("Rigidbody"))
			{
				 //Get Component for this entity, if it exists
				RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(entity);
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);

				if (rigidbody.bFlagCreated)
				{
					InitRigidbody(rigidbody, transform);
					rigidbody.bFlagCreated = false;
				}

				if (rigidbody.bFlagDeleted || world->IsDeleted(entity))
				{
					CleanupRigidbody(rigidbody);
					world->RemoveComponent<RigidbodyComponent>(entity);
				}
			}

			if (dt > 0.0f)
			{
				// Step Physics Simulation
				physicsWorld->stepSimulation(dt);

				// Update Entity transfrom from Physics Component
				for (ECS::Entity entity : entityMap.at("Rigidbody"))
				{
					TransformComponent& worldTransform = world->GetComponent<TransformComponent>(entity);
					RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(entity);

					if (rigidbody.mass > 0.0f)
					{
						btTransform physicsTransform;

						if (rigidbody.body && rigidbody.body->getMotionState())
						{
							rigidbody.body->getMotionState()->getWorldTransform(physicsTransform);
						}

						// Update Transform Component Poisiton/Rotation
						worldTransform.position = physicsTransform.getOrigin();

						// Convert Rotation from Radians to Degrees
						physicsTransform.getRotation().getEulerZYX(worldTransform.rotation.z, worldTransform.rotation.y, worldTransform.rotation.x);

						// Convert from Radians to Degrees
						float PI = 3.14159;
						worldTransform.rotation.x *= 180 / PI;
						worldTransform.rotation.y *= 180 / PI;
						worldTransform.rotation.z *= 180 / PI;
					}
				}
			}
		}

		void BulletPhysicsSystem::InitRigidbody(RigidbodyComponent& rigidbody, TransformComponent& worldTransform)
		{
			// Construct Box Shape for Component
			rigidbody.shape = new btBoxShape(rigidbody.size);

			// Store shape for potential re-use
			collisionShapes.push_back(rigidbody.shape);

			// Initialise Component with Starting Position
			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(worldTransform.position);

			// Set Body to dynamic if it has any mass
			bool isDynamic = (rigidbody.mass != 0.0f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
				rigidbody.shape->calculateLocalInertia(rigidbody.mass, localInertia);

			// MotionState is used for interpolation and syncing active objects
			btDefaultMotionState* motionState = new btDefaultMotionState(transform);

			// Create Rigid Body
			btRigidBody::btRigidBodyConstructionInfo rbInfo(rigidbody.mass, motionState, rigidbody.shape, localInertia);
			rigidbody.body = new btRigidBody(rbInfo);

			physicsWorld->addRigidBody(rigidbody.body);
		}

		void BulletPhysicsSystem::CleanupRigidbody(RigidbodyComponent& rigidbody)
		{

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
			//RigidbodyEvent rigidbodyEvent;
			//while (!rigidbodyEvents->IsEmpty())
			//{
			//	if (rigidbodyEvents->Pop(rigidbodyEvent))
			//	{
			//		// Get Component for this entity, if it exists
			//		RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(rigidbodyEvent.entity);
			//		TransformComponent& transform = world->GetComponent<TransformComponent>(rigidbodyEvent.entity);

			//		if (rigidbodyEvent.shouldCreate)
			//		{
			//			CleanupComponent(rigidbodyEvent.entity);
			//			InitComponent(rigidbodyEvent.entity, rigidbody.size, rigidbody.mass, transform.position);
			//		}

			//		if (rigidbodyEvent.shouldDelete)
			//		{
			//			CleanupComponent(rigidbodyEvent.entity);
			//			world->RemoveComponent<RigidbodyComponent>(rigidbodyEvent.entity);
			//		}
			//	}
			//}
		}
	}
}
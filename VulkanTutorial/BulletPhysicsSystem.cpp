#include "BulletPhysicsSystem.h"
#include "TransformComponent.h"
#include "VectorConversion.h"

namespace Puffin
{
	namespace Physics
	{
		void BulletPhysicsSystem::Start()
		{
			// Initialise Bullet Physics Objects
			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			broadphaseInterface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver;
			physicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphaseInterface, solver, collisionConfig);

			physicsWorld->setGravity(btVector3(0, -9.81f, 0));

			//TransformComponent& comp1 = world->GetComponent<TransformComponent>(3);
			//TransformComponent& comp2 = world->GetComponent<TransformComponent>(5);

			//InitComponent(3, btVector3(1.0f, 1.0f, 1.0f), 1.0f, btVector3(comp1.position.x, comp1.position.y, comp1.position.z));
			//InitComponent(5, btVector3(1.0f, 1.0f, 1.0f), 0.0f, btVector3(comp2.position.x, comp2.position.y, comp2.position.z));

			for (ECS::Entity entity : entities)
			{
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
				RigidbodyComponent& rigidbody = world->GetComponent<RigidbodyComponent>(entity);
				InitComponent(entity, rigidbody.size, rigidbody.mass, transform.position);
			}
		}

		void BulletPhysicsSystem::Stop()
		{
			for (ECS::Entity entity : entities)
			{
				CleanupComponent(entity);

				world->RemoveComponent<RigidbodyComponent>(entity);
			}

			entities.clear();

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
			// Step Physics Simulation
			physicsWorld->stepSimulation(dt);

			// Update Entity transfrom from Physics Component
			for (ECS::Entity entity : entities)
			{
				TransformComponent& transformComp = world->GetComponent<TransformComponent>(entity);
				RigidbodyComponent& physicsComp = world->GetComponent<RigidbodyComponent>(entity);

				// Init/Recreate Flagged Components
				if (physicsComp.flag_created)
				{
					InitComponent(entity, physicsComp.size, physicsComp.mass, transformComp.position);
					physicsComp.flag_created = false;
				}

				// Destroy Dlagged Components
				if (physicsComp.flag_deleted)
				{
					CleanupComponent(entity);
					world->RemoveComponent<RigidbodyComponent>(entity);
				}

				btTransform transform;

				if (physicsComp.body && physicsComp.body->getMotionState())
				{
					physicsComp.body->getMotionState()->getWorldTransform(transform);
				}

				// Update Transform Component Poisiton/Rotation
				transformComp.position = transform.getOrigin();
				transform.getRotation().getEulerZYX(transformComp.rotation.z, transformComp.rotation.y, transformComp.rotation.x);
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

		BulletPhysicsSystem::~BulletPhysicsSystem()
		{
			Stop();
		}
	}
}
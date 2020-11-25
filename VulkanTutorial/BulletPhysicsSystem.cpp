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

			TransformComponent& comp1 = world->GetComponent<TransformComponent>(3);
			TransformComponent& comp2 = world->GetComponent<TransformComponent>(5);

			InitComponent(3, btVector3(1.0f, 1.0f, 1.0f), 1.0f, btVector3(comp1.position.x, comp1.position.y, comp1.position.z));
			InitComponent(5, btVector3(1.0f, 1.0f, 1.0f), 0.0f, btVector3(comp2.position.x, comp2.position.y, comp2.position.z));
		}

		void BulletPhysicsSystem::Stop()
		{
			int i = 0;

			for (ECS::Entity entity : entities)
			{
				btCollisionObject* obj = physicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);

				if (body && body->getMotionState())
				{
					delete body->getMotionState();
				}

				physicsWorld->removeCollisionObject(obj);
				delete obj;

				i++;
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
			// Step Physics Simulation
			physicsWorld->stepSimulation(dt);

			int i = 0;

			// Update Entity transfrom from Physics Component
			for (ECS::Entity entity : entities)
			{
				TransformComponent& transformComp = world->GetComponent<TransformComponent>(entity);
				BulletPhysicsComponent& physicsComp = world->GetComponent<BulletPhysicsComponent>(entity);

				btTransform transform;

				if (physicsComp.body && physicsComp.body->getMotionState())
				{
					physicsComp.body->getMotionState()->getWorldTransform(transform);
				}

				// Update Transform Component Poisiton/Rotation
				transformComp.position = transform.getOrigin();
				transform.getRotation().getEulerZYX(transformComp.rotation.z, transformComp.rotation.y, transformComp.rotation.x);

				float PI = 3.14159;

				// Convert stored rotation values from radians to degrees
				transformComp.rotation.x = transformComp.rotation.x * 180 / PI;
				transformComp.rotation.y = transformComp.rotation.y * 180 / PI;
				transformComp.rotation.z = transformComp.rotation.z * 180 / PI;

				i++;
			}
		}

		void BulletPhysicsSystem::InitComponent(ECS::Entity entity, btVector3 size, btScalar mass, btVector3 position)
		{
			// Get Component for this entity, if it exists
			BulletPhysicsComponent& comp = world->GetComponent<BulletPhysicsComponent>(entity);

			// Construct Box Shape for Component
			comp.shape = new btBoxShape(size);

			// Store shape for potential re-use
			collisionShapes.push_back(comp.shape);

			// Initialise Component with Starting Position
			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(position);

			// Set Body to dynamic if it has any mass
			bool isDynamic = (mass != 0.0f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
				comp.shape->calculateLocalInertia(mass, localInertia);

			// MotionState is used for interpolation and syncing active objects
			btDefaultMotionState* motionState = new btDefaultMotionState(transform);
			
			// Create Rigid Body
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, comp.shape, localInertia);
			comp.body = new btRigidBody(rbInfo);

			physicsWorld->addRigidBody(comp.body);
		}

		BulletPhysicsSystem::~BulletPhysicsSystem()
		{
			Stop();
		}
	}
}
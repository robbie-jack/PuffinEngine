#include "PhysicsSystem.h"

#include <glm/glm.hpp>
#include <iostream>

void PhysicsSystem::Init()
{
	running = true;

	CreateDynamicsWorld();
}

bool PhysicsSystem::Update(float dt)
{
	if (dynamicsWorld)
	{
		dynamicsWorld->stepSimulation(dt);
	}

	return running;
}

void PhysicsSystem::SendMessage()
{

}

PhysicsComponent* PhysicsSystem::AddComponent()
{
	PhysicsComponent physics_component;
	physicsComponents.push_back(physics_component);
	return &physicsComponents.back();
}

void PhysicsSystem::InitComponent(int handle, float mass, Transform transform, btCollisionShape* shape)
{
	physicsComponents[handle].SetTransform(ConvertTransform(transform));
	physicsComponents[handle].SetShape(shape);
	physicsComponents[handle].SetRigidBody(CreateRigidBody(mass, physicsComponents[handle].GetTransform(), physicsComponents[handle].GetShape()));
}

btBoxShape* PhysicsSystem::CreateBoxShape(glm::vec3 halfExtents)
{
	// Create and return new box shape from coverted half extent
	btBoxShape* box = new btBoxShape(ConvertVector(halfExtents));
	return box;
}

btVector3 PhysicsSystem::ConvertVector(glm::vec3 vector)
{
	// Convert vector from glm to bullet vector
	return btVector3(vector.x, vector.y, vector.z);
}

btTransform PhysicsSystem::ConvertTransform(Transform transform)
{
	btTransform bt_transform;
	bt_transform.setOrigin(ConvertVector(transform.position));
	bt_transform.setRotation(btQuaternion(transform.rotation.y, transform.rotation.x, transform.rotation.z));

	return bt_transform;
}

void PhysicsSystem::CreateDynamicsWorld()
{
	collisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	broadphase = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	solver = sol;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	if (dynamicsWorld == nullptr)
		std::cout << "Failed to create physics world" << std::endl;
	else
		dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

btRigidBody* PhysicsSystem::CreateRigidBody(float mass, btTransform transform, btCollisionShape* shape)
{
	// Rigidbody is dynamic if mass = 0
	bool isDynamic = (mass != 0.0f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		shape->calculateLocalInertia(mass, localInertia);

#define USE_MOTIONSTATE 1
#ifdef USE_MOTIONSTATE
	btDefaultMotionState* myMotionState = new btDefaultMotionState(transform);

	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);

	btRigidBody* body = new btRigidBody(cInfo);
#else
	btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
	body->setWorldTransform(transform);
#endif
	body->setUserIndex(-1);
	dynamicsWorld->addRigidBody(body);
	return body;
}

PhysicsSystem::~PhysicsSystem()
{
	// Remove rigidbodies from dynamics world and delete
	if (dynamicsWorld)
	{
		int i;
		for (i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
		{
			dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
		}

		for (i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}
	}

	// Delte collision shapes
	for (int j = 0; j < collisionShapes.size(); j++)
	{
		btCollisionShape* shape = &collisionShapes[j];
		delete shape;
	}
	collisionShapes.clear();

	delete dynamicsWorld;
	dynamicsWorld = 0;

	delete solver;
	solver = 0;

	delete broadphase;
	broadphase = 0;

	delete dispatcher;
	dispatcher = 0;

	delete collisionConfiguration;
	collisionConfiguration = 0;
}
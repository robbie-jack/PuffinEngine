#pragma once

#include "System.h"
#include "btBulletDynamicsCommon.h"
#include "PhysicsComponent.h"
#include "Transform.h"

#include <vector>

class PhysicsSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	PhysicsComponent* AddComponent(); // Add Physics Component
	void InitComponent(int handle, float mass, Transform transform, btCollisionShape* shape); // Initialise Component and create rigidbody

	static btBoxShape* CreateBoxShape(glm::vec3 halfExtents);
	
	static inline btVector3 ConvertVector(glm::vec3 vector); // Covert from glm vector to bullet vector
	static inline btTransform ConvertTransform(Transform transform); // Build a bullet transform from Transfrom struct

	~PhysicsSystem();

private:

	btDiscreteDynamicsWorld* dynamicsWorld;
	btBroadphaseInterface* broadphase;
	btCollisionDispatcher* dispatcher;
	btConstraintSolver* solver;
	btDefaultCollisionConfiguration* collisionConfiguration;

	btAlignedObjectArray<btCollisionShape> collisionShapes;
	std::vector<PhysicsComponent> physicsComponents;

	void CreateDynamicsWorld();
	btRigidBody* CreateRigidBody(float mass, btTransform transform, btCollisionShape* shape);
};
#include "ReactPhysicsSystem.h"

#include <cmath>

void ReactPhysicsSystem::Init()
{
	running = true;

	gravity = rp3d::Vector3(0.0f, -9.8f, 0.0f);
	dynamicsWorld = new rp3d::DynamicsWorld(gravity);

	timeSinceLastUpdate = 0.0f;

	InitComponent(0, rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::Vector3(0.0f, 0.0f, 0.0f));
}

bool ReactPhysicsSystem::Update(float dt)
{
	// Add Frame Time onto timeSinceLastUpdate value
	timeSinceLastUpdate += dt;

	// Update Physics World when timeStep is exceeded
	if (timeSinceLastUpdate >= timeStep)
	{
		// Update Physics world with fixed time step
		dynamicsWorld->update(timeStep);

		// Decrease time since last update
		timeSinceLastUpdate -= timeStep;
	}

	decimal factor = timeSinceLastUpdate / timeStep;

	for (int i = 0; i < physicsComponents.size(); i++)
	{
		// Get current transform from dynamics world
		rp3d::Transform currTransform = physicsComponents[i].GetTransform();

		// Calculate Interpolated Transform between previous and current transforms
		rp3d::Transform lerpTransform = Transform::interpolateTransforms(physicsComponents[i].GetPrevTransform(), currTransform, factor);

		// Set interpolated transform on component so it can be retrieved by transform system
		physicsComponents[i].SetLerpTransform(lerpTransform);

		// Set previous transfrom to current transform for this frame
		physicsComponents[i].SetPrevTransform(currTransform);
	}

	return running;
}

void ReactPhysicsSystem::SendMessage()
{

}

ReactPhysicsComponent* ReactPhysicsSystem::AddComponent()
{
	ReactPhysicsComponent physicsComponent;
	physicsComponents.push_back(physicsComponent);
	return &physicsComponents.back();
}

ReactPhysicsComponent* ReactPhysicsSystem::GetComponent(uint32_t entityID)
{
	for (auto comp : physicsComponents)
	{
		if (comp.GetEntityID() == entityID)
		{
			return &comp;
		}
	}
}

void ReactPhysicsSystem::InitComponent(int handle, rp3d::Vector3 position, rp3d::Vector3 rotation)
{
	// Create Quaternion/Transform of new rigid body
	rp3d::Quaternion initQuaternion = rp3d::Quaternion::fromEulerAngles(rotation);
	rp3d::Transform transform(position, initQuaternion);

	// Create Rigid Body with dynamics world and store in component
	physicsComponents[handle].SetBody(dynamicsWorld->createRigidBody(transform));
	physicsComponents[handle].SetPrevTransform(transform);
}

ReactPhysicsSystem::~ReactPhysicsSystem()
{
	// Destory All Rigid Bodies
	for (auto comp : physicsComponents)
	{
		dynamicsWorld->destroyRigidBody(comp.GetBody());
	}

	// Clear Components Vector
	physicsComponents.clear();

	// Destroy Dynamics World
	delete dynamicsWorld;
	dynamicsWorld = NULL;
}
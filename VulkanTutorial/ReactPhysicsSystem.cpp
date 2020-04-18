#include "ReactPhysicsSystem.h"

void ReactPhysicsSystem::Init()
{
	running = true;

	gravity = rp3d::Vector3(0.0f, -9.81f, 0.0f);
	dynamicsWorld = new rp3d::DynamicsWorld(gravity);

	timeSinceLastUpdate = 0.0f;
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

	return running;
}

void ReactPhysicsSystem::SendMessage()
{

}

ReactPhysicsSystem::~ReactPhysicsSystem()
{
	// Destroy Dynamics World
	delete dynamicsWorld;
	dynamicsWorld = NULL;
}
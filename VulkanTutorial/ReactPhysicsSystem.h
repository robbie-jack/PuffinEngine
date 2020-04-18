#pragma once

#include "System.h"

#include "reactphysics3d.h"

using namespace reactphysics3d;

class ReactPhysicsSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	~ReactPhysicsSystem();

private:

	const float timeStep = 1.0f / 60.0f;
	float timeSinceLastUpdate;

	rp3d::Vector3 gravity;
	rp3d::DynamicsWorld* dynamicsWorld;
};
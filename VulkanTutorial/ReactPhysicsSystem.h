#pragma once

#include "System.h"

#include "reactphysics3d.h"
#include "ReactPhysicsComponent.h"
#include "TransformSystem.h"

using namespace reactphysics3d;

class ReactPhysicsSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	ReactPhysicsComponent* AddComponent();
	ReactPhysicsComponent* GetComponent(uint32_t entityID);
	void InitComponent(int handle, rp3d::Vector3 position, rp3d::Vector3 rotation);

	inline std::vector<ReactPhysicsComponent>* GetComponents() { return &physicsComponents; };

	~ReactPhysicsSystem();

private:

	const float timeStep = 1.0f / 60.0f;
	float timeSinceLastUpdate;

	rp3d::Vector3 gravity;
	rp3d::DynamicsWorld* dynamicsWorld;

	std::vector<ReactPhysicsComponent> physicsComponents;
};
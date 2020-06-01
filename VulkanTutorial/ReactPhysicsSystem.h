#pragma once

#include "System.h"

#include "reactphysics3d/reactphysics3d.h"
#include "ReactPhysicsComponent.h"
#include "TransformSystem.h"

class ReactPhysicsSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	ReactPhysicsComponent* AddComponent();
	ReactPhysicsComponent* GetComponent(uint32_t entityID);
	void InitComponent(int handle, rp3d::Vector3 position = rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::Vector3 rotation = rp3d::Vector3(0.0f, 0.0f, 0.0f), BodyType bodyType = BodyType::DYNAMIC);

	inline std::vector<ReactPhysicsComponent>* GetComponents() { return &physicsComponents; };

	~ReactPhysicsSystem();

private:

	const float timeStep = 1.0f / 60.0f;
	float timeSinceLastUpdate;

	rp3d::PhysicsCommon physicsCommon;
	rp3d::PhysicsWorld* physicsWorld;

	std::vector<ReactPhysicsComponent> physicsComponents;
};
#pragma once

#include "System.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "ReactPhysicsComponent.h"

#include "VectorConversion.h"

#include <vector>

class TransformSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	TransformComponent* AddComponent();
	TransformComponent* GetComponent(uint32_t entityID);

	inline void SetPhysicsRenderVectors(std::vector<ReactPhysicsComponent>* physicsComponents_, std::vector<RenderComponent>* renderComponents_)
	{
		physicsComponents = physicsComponents_;
		renderComponents = renderComponents_;
	}

	~TransformSystem();

private:

	std::vector<TransformComponent> transformComponents;
	std::vector<ReactPhysicsComponent>* physicsComponents;
	std::vector<RenderComponent>* renderComponents;
};
#pragma once

#include "System.h"
#include "TransformComponent.h"

#include <vector>

class TransformSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	TransformComponent* AddComponent();
	TransformComponent* GetComponent(uint32_t entityID);

	~TransformSystem();

private:

	std::vector<TransformComponent> transformComponents;

};
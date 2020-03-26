#pragma once

//#include "System.h"
#include "RenderComponent.h"

#include <vector>

class Entity
{
public:

	Entity(uint32_t id);

	void AttachComponent(BaseComponent* component);

	template <class T>
	void DetachComponent();

	inline uint32_t GetID() { return entityID; };

protected:
	uint32_t entityID;

	// Array of handles to components that are attached to enitity
	std::vector<BaseComponent*> entityComponents;

	RenderComponent* GetRenderComponent();
};
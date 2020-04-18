#include "TransformSystem.h"

void TransformSystem::Init()
{

}

bool TransformSystem::Update(float dt)
{
	return true;
}

void TransformSystem::SendMessage()
{

}

TransformComponent* TransformSystem::AddComponent()
{
	TransformComponent transformComponent;
	transformComponents.push_back(transformComponent);
	return &transformComponents.back();
}

TransformComponent* TransformSystem::GetComponent(uint32_t entityID)
{
	for (auto comp : transformComponents)
	{
		if (comp.GetEntityID() == entityID)
		{
			return &comp;
		}
	}
}

TransformSystem::~TransformSystem()
{
	transformComponents.clear();
}
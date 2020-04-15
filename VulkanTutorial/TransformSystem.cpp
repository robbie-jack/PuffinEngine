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

TransformSystem::~TransformSystem()
{
	transformComponents.clear();
}
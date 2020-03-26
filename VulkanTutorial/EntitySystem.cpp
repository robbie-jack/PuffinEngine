#include "EntitySystem.h"

void EntitySystem::Init()
{
	nextID = 1;
}

bool EntitySystem::Update(float dt)
{
	return true;
}

void EntitySystem::SendMessage()
{

}

uint32_t EntitySystem::CreateEntity()
{
	Entity entity(nextID);
	entityMap.insert(std::pair<uint32_t, Entity>(nextID, entity));
	nextID++;

	return entity.GetID();
}

Entity* EntitySystem::GetEntity(uint32_t entityID)
{
	return &entityMap.find(entityID)->second;
}

EntitySystem::~EntitySystem()
{

}
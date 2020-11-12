#include "EntitySystem.h"

namespace Puffin
{
	void EntitySystem::Init()
	{
		//running = true;
		//updateWhenPlaying = false;
		//type = SystemType::ENTITY;
	}

	void EntitySystem::Start()
	{

	}

	bool EntitySystem::Update(float dt)
	{
		return true;
	}

	void EntitySystem::Stop()
	{

	}

	void EntitySystem::SendMessage()
	{

	}

	uint32_t EntitySystem::CreateEntity()
	{
		Entity entity(nextID);
		entityMap.insert(std::pair<uint32_t, Entity>(nextID, entity));
		nextID++;

		UpdateIDVector();

		return entity.GetID();
	}

	void EntitySystem::DestroyEntity(uint32_t entityID)
	{
		entityMap.erase(entityID);

		UpdateIDVector();
	}

	Entity* EntitySystem::GetEntity(uint32_t entityID)
	{
		return &entityMap.find(entityID)->second;
	}

	EntitySystem::~EntitySystem()
	{

	}

	void EntitySystem::UpdateIDVector()
	{
		entityIDVector.clear();

		for (auto entity : entityMap)
		{
			entityIDVector.push_back(entity.first);
		}
	}
}
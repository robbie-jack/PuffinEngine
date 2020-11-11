#include "EntityManager.h"

namespace Puffin
{
	namespace ECS
	{
		void EntityManager::Init()
		{
			for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
			{
				availableEntities.push(entity);
			}

			activeEntityCount = 0;
		}

		Entity EntityManager::CreateEntity()
		{
			Entity entity = availableEntities.front();
			availableEntities.pop();

			activeEntityCount++;

			return entity;
		}

		void EntityManager::DestroyEntity(Entity entity)
		{
			availableEntities.push(entity);
			activeEntityCount--;
		}
	}
}
#pragma once

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <string>
#include <cassert>

namespace Puffin
{
	namespace ECS
	{
		typedef uint32_t Entity;

		const Entity MAX_ENTITIES = 5000;

		class EntityManager
		{
		public:

			void Init();
			Entity CreateEntity();
			void DestroyEntity(Entity entity);

			inline std::string GetName(Entity entity) 
			{ 
				assert(entityNames.find(Entity) != entityNames.end() && "Attempting to get name of nonexistant entity");
				return entityNames[entity]; 
			};
			inline void SetName(Entity entity, std::string name) 
			{ 
				assert(entityNames.find(Entity) != entityNames.end() && "Attempting to set name of nonexistant entity");
				entityNames[entity] = name;
			}

		private:

			std::queue<Entity> availableEntities;
			std::unordered_map<Entity, std::string> entityNames;
			uint32_t activeEntityCount;

		};
	}
}
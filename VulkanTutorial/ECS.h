#pragma once

#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <set>

namespace Puffin
{
	namespace ECS
	{
		typedef uint32_t Entity;
		const Entity MAX_ENTITIES = 5000;

		static const Entity INVALID_ENTITY = 0;

		typedef uint8_t ComponentType;
		const ComponentType MAX_COMPONENTS = 32;

		typedef std::bitset<MAX_COMPONENTS> Signature;

		class EntityManager
		{
		public:

			EntityManager()
			{
				for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
				{
					availableEntities.push(entity);
				}
			}

			Entity CreateEntity()
			{
				assert(activeEntityCount < MAX_ENTITIES && "Max number of allowed entities reached");

				// Get next available ID from queue
				Entity entity = availableEntities.front();
				availableEntities.pop();
				activeEntityCount++;

				activeEntities.insert(entity);

				return entity;
			}

			void DestroyEntity(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Reset signature for this entity
				entitySignatures[entity].reset();

				activeEntities.erase(entity);

				// Add id to back of queue
				availableEntities.push(entity);
				activeEntityCount--;
			}

			void SetSignature(Entity entity, Signature signature)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Update this entity's signature
				entitySignatures[entity] = signature;
			}

			Signature GetSignature(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Get entity signature
				return entitySignatures[entity];
			}

			std::set<Entity> GetActiveEntities()
			{
				return activeEntities;
			}

		private:

			std::queue<Entity> availableEntities;
			std::set<Entity> activeEntities;
			std::array<Signature, MAX_ENTITIES> entitySignatures;

			uint32_t activeEntityCount;
		};
	}
}
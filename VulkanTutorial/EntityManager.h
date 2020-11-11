#pragma once

#include <cstdint>
#include <queue>

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

		private:

			std::queue<Entity> availableEntities;
			uint32_t activeEntityCount;

		};
	}
}
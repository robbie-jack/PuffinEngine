#pragma once

#include <unordered_map>
#include <set>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;
		typedef uint32_t Entity;

		typedef std::unordered_map<std::string_view, std::set<Entity>> EntityMap;

		class System
		{
		public:

			EntityMap entityMap;
			std::shared_ptr<World> world;

		protected:


		private:


		};
	}
}
#pragma once

#include <set>
#include <memory>
#include <bitset>
#include <unordered_map>

#include "ComponentManager.h"

namespace Puffin
{
	namespace ECS
	{
		//typedef uint32_t Entity;
		//typedef uint32_t ComponentType;

		//const ComponentType MAX_COMPONENTS = 32;

		typedef std::bitset<MAX_COMPONENTS> Signature;

		////////////////////////////////////////
		// System
		////////////////////////////////////////

		class SystemManager;

		class System
		{
		public:
			std::set<Entity> entities;
			std::shared_ptr<SystemManager> systemManager;
		};

		////////////////////////////////////////
		// System Manager
		////////////////////////////////////////

		class SystemManager
		{
		public:

			template<typename System>
			std::shared_ptr<System> RegisterSystem();

			template<typename System>
			void SetSignature(Signature signature);

			void EntityDestroyed(Entity entity);

			void EntitySignatureChanged(Entity entity, Signature entitySignature);

			std::shared_ptr<ComponentManager> componentManager;

		private:

			std::unordered_map<const char*, Signature> signatures;
			std::unordered_map<const char*, std::shared_ptr<System>> systems;

		};
	}
}
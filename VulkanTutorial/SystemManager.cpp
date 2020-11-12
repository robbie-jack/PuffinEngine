#include "SystemManager.h"

#include <cassert>
#include <typeinfo>

namespace Puffin
{
	namespace ECS
	{
		////////////////////////////////////////
		// System Manager
		////////////////////////////////////////

		template<typename System>
		std::shared_ptr<System> SystemManager::RegisterSystem()
		{
			const char* typeName = typeid(System).name();

			assert(mSystems.find(typeName) == systems.end() && "Registering system more than once.");

			// Create and return pointer to system
			auto system = std::make_shared<System>();
			systems.insert({ typeName, system });
			return system;
		}

		template<typename System>
		void SystemManager::SetSignature(Signature signature)
		{
			const char* typeName = typeid(System).name();

			assert(mSystems.find(typeName) != systems.end() && "System used before being registered");

			// Set Signature for this system
			signatures.insert({ typeName, signature });
		}

		void SystemManager::EntityDestroyed(Entity entity)
		{
			// Erase destroyed entity from all systems
			for (auto const& pair : systems)
			{
				auto const& system = pair.second;
				system->entities.erase(entity);
			}
		}

		void SystemManager::EntitySignatureChanged(Entity entity, Signature entitySignature)
		{
			// Notify each system that entity signature has changed
			for (auto const& pair : systems)
			{
				auto const& type = pair.first;
				auto const& system = pair.second;
				auto const& systemSignature = signatures[type];

				// Entity signature matches system signature - insert into set
				if ((entitySignature & systemSignature) == systemSignature)
				{
					system->entities.insert(entity);
				}
				// signature does not match system signature - erase from set
				else
				{
					system->entities.erase(entity);
				}
			}
		}
	}
}
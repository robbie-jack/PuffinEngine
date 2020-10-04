#pragma once

#include "System.h"
#include "Entity.h"

#include <map>

namespace Puffin
{
	class EntitySystem : public System
	{
	public:
		void Init();
		void Start();
		bool Update(float dt);
		void Stop();
		void SendMessage();

		uint32_t CreateEntity();
		Entity* GetEntity(uint32_t);

		~EntitySystem();
	private:
		uint32_t nextID = 1;
		std::map<uint32_t, Entity> entityMap;
	};
}
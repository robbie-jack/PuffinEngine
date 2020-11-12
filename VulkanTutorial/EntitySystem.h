#pragma once

#include "System.h"
#include "Entity.h"

#include <map>

namespace Puffin
{
	class EntitySystem
	{
	public:
		void Init();
		void Start();
		bool Update(float dt);
		void Stop();
		void SendMessage();

		uint32_t CreateEntity();
		void DestroyEntity(uint32_t entityID);
		Entity* GetEntity(uint32_t entityID);

		inline std::vector<uint32_t> GetEntityIDVector() { return entityIDVector; };

		~EntitySystem();
	private:
		uint32_t nextID = 1;
		std::map<uint32_t, Entity> entityMap;
		std::vector<uint32_t> entityIDVector;

		void UpdateIDVector();
	};
}
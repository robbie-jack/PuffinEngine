#pragma once

#include "stdint.h"
#include <string>

namespace Puffin
{
	enum class ComponentType
	{
		TRANSFORM,
		MESH,
		PHYSICS
	};

	class BaseComponent
	{
	public:
		virtual ~BaseComponent() {};

		inline ComponentType GetType() { return type; };
		inline std::string GetName() { return name; };
		inline uint32_t GetEntityID() { return entityID; };
		inline void SetEntityID(uint32_t id) { entityID = id; };

	protected:
		ComponentType type;
		std::string name;
		uint32_t entityID;
	};
}
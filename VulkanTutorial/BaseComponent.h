#pragma once

#include "stdint.h"

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
	inline uint32_t GetEntityID() { return entityID; };
	inline void SetEntityID(uint32_t id) { entityID = id; };

protected:
	ComponentType type;
	uint32_t entityID;
};
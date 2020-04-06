#pragma once

enum class ComponentType
{
	TRANSFORM,
	RENDER,
	PHYSICS
};

class BaseComponent
{
public:
	virtual ~BaseComponent() {};

	inline ComponentType GetType() { return type; };
	inline int GetEntityID() { return entityID; };

protected:
	ComponentType type;
	int entityID;
};
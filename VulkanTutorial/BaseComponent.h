#pragma once

enum class ComponentType
{
	RENDER
};

class BaseComponent
{
public:
	virtual ~BaseComponent() {};

	inline ComponentType GetType() { return type; };

protected:
	ComponentType type;
};
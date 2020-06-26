#pragma once

namespace Puffin
{
	enum class SystemType
	{
		ENTITY,
		RENDER,
		PHYSICS
	};

	class System
	{
	public:

		virtual void Init() = 0;

		virtual bool Update(float dt) = 0;

		virtual void SendMessage() = 0;

		inline SystemType GetType() { return type; };

		virtual ~System() {};

	protected:
		bool running;
		SystemType type;
	};
}
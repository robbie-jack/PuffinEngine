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

		virtual void Start() = 0;

		virtual bool Update(float dt) = 0;

		virtual void Stop() = 0;

		virtual void SendMessage() = 0;

		inline SystemType GetType() { return type; };
		inline bool GetUpdateWhenPlaying() { return updateWhenPlaying; };

		virtual ~System() {};

	protected:
		bool running;
		bool updateWhenPlaying; // Flag to indicate system should only update when game is playing
		SystemType type;
	};
}
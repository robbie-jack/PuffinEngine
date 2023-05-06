#pragma once

#include "Engine/Subsystem.h"

#include <string>

namespace puffin::core
{
	class Engine;

	// Info about the system
	struct SystemInfo
	{
		std::string name;
	};

	//////////////////////////////////////////////////
	// System
	//////////////////////////////////////////////////

	class World;

	class System : public core::Subsystem
	{
	public:

		~System() override
		{
			mEngine = nullptr;
		}

		// Get struct with info on system such as its update order
		const core::SystemInfo& getInfo() { return mSystemInfo; }

	protected:

		core::SystemInfo mSystemInfo;
	};
}

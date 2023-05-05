#pragma once

#include "Engine/Subsystem.hpp"

#include <string>
#include <memory>

namespace puffin
{
	namespace core
	{
		class Engine;

		// Info about the system
		struct SystemInfo
		{
			std::string name;
		};
	}

	namespace ECS
	{
		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;

		class System : public core::Subsystem
		{
		public:

			~System() override
			{
				mWorld = nullptr;
				mEngine = nullptr;
			}
			
			// Get struct with info on system such as its update order
			const core::SystemInfo& getInfo() { return mSystemInfo; }

			void setWorld(const std::shared_ptr<World>& world)
			{
				mWorld = world;
			}

		protected:

			std::shared_ptr<World> mWorld = nullptr;

			core::SystemInfo mSystemInfo;

		private:


		};
	}
}
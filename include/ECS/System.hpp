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
				m_world = nullptr;
				m_engine = nullptr;
			}
			
			// Get struct with info on system such as its update order
			const core::SystemInfo& GetInfo() { return m_systemInfo; } const

			void SetWorld(std::shared_ptr<World> inWorld)
			{
				m_world = inWorld;
			}

		protected:

			std::shared_ptr<World> m_world = nullptr;

			core::SystemInfo m_systemInfo;

		private:


		};
	}
}
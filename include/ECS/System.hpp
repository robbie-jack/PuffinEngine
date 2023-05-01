#pragma once

#include "ECS/EntityID.h"

#include <unordered_map>
#include <set>
#include <memory>
#include <string_view>

namespace Puffin
{
	namespace Core
	{
		class Engine;

		// Various stages when methods can be executed during engine runtime
		enum class ExecutionStage
		{
			Idle,			// Only used for calculating idle time when frame rate is limited, do not use with callback
			Init,			// Occurs once on engine launch, use for one off system initialization
			Setup,			// Occurs on engine launch and whenever gameplay is stopped
			Start,			// Occurs whenever gameplay is started
			FixedUpdate,	// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
			Update,			// Update once a frame - Useful for non-determinstic gameplay code
			EditorUI,		// Update once a frame - Used for updating editor UI prior to rendering
			PreRender,		// Update once a frame - Use for code which should be ran before rendering
			Render,			// Update once a frame - Useful for code which relates to the rendering pipeline
			Stop,			// Occurs when game play is stopped, use for resetting any gameplay data
			Cleanup			// Occurs when engine exits, use for cleaning up all data
		};

		const std::vector<std::pair<ExecutionStage, const std::string>> G_EXECUTION_STAGE_ORDER =
		{
			{ ExecutionStage::Idle, "Idle" },
			{ ExecutionStage::FixedUpdate, "FixedUpdate" },
			{ ExecutionStage::Update, "Update" },
			{ ExecutionStage::Render, "Render" },
		};

		// Info about the system
		struct SystemInfo
		{
			std::string name;
			ExecutionStage updateOrder;
		};
	}

	namespace ECS
	{
		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;

		typedef std::unordered_map<std::string_view, std::set<EntityID>> EntityMap;

		class System
		{
		public:

			virtual ~System()
			{
				m_world = nullptr;
				m_engine = nullptr;
			}

			// Virtual functions to be called by ECSWorld
			virtual void SetupCallbacks() = 0; // Called to setup this systems callbacks when it is registered
			
			// Get struct with info on system such as its update order
			const Core::SystemInfo& GetInfo() { return m_systemInfo; } const

			void SetEngine(std::shared_ptr<Core::Engine> inEngine)
			{
				m_engine = inEngine;
			}

			void SetWorld(std::shared_ptr<World> inWorld)
			{
				m_world = inWorld;
			}

		protected:

			std::shared_ptr<Core::Engine> m_engine = nullptr;
			std::shared_ptr<World> m_world = nullptr;

			Core::SystemInfo m_systemInfo;

		private:


		};
	}
}
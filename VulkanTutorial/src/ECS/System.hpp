#pragma once

#include <unordered_map>
#include <set>
#include <memory>
#include <string_view>

namespace Puffin
{
	namespace Core
	{
		class Engine;

		// Enum for setting when a system's update function should be called
		enum class UpdateOrder
		{
			None = 0,		// Do not perform updates for this system
			FixedUpdate,	// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
			Update,			// Update once a frame - Useful for non-determinstic gameplay code
			PreRender,		// Update once a frame - Use for code which should be ran before rendering
			Render			// Update once a frame - Useful for code which relates to the rendering pipeline
		};

		// Info about the system
		struct SystemInfo
		{
			std::string name;
			UpdateOrder updateOrder = UpdateOrder::None;
		};
	}

	namespace ECS
	{
		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;
		class ECSSubsystem;

		typedef std::unordered_map<std::string_view, std::set<EntityID>> EntityMap;

		class System
		{
		public:

			virtual ~System()
			{
				m_world = nullptr;
				m_engine = nullptr;
			}

			EntityMap entityMap;

			// Virtual functions to be called by ECSWorld
			virtual void Init() = 0;		// Called when engine starts
			virtual void PreStart() = 0;	// Called prior to gameplay starting to do some pre-start initialization
			virtual void Start() = 0;		// Called when gameplay starts
			virtual void Update() = 0;		// Called each frame, depending on what update order is set
			virtual void Stop() = 0;		// Called when gameplay ends
			virtual void Cleanup() = 0;		// Called when engine exits
			
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

			void SetECS(std::shared_ptr<ECSSubsystem> inECS)
			{
				m_ecs = inECS;
			}

		protected:

			std::shared_ptr<Core::Engine> m_engine = nullptr;
			std::shared_ptr<World> m_world = nullptr;
			std::shared_ptr<ECSSubsystem> m_ecs = nullptr;

			Core::SystemInfo m_systemInfo;

		private:


		};
	}
}
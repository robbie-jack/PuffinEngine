#pragma once

#include <unordered_map>
#include <set>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		// Enum for setting when a system's update function should be called
		enum class UpdateOrder
		{
			None = 0,		// Do not perform updates for this system
			FixedUpdate,	// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
			Update,			// Update occurs once a frame - Useful for non-determinstic gameplay code
			Rendering		// Update once a frame - Useful for code which relates to the rendering pipeline
		};

		// Info about the system
		struct SystemInfo
		{
			UpdateOrder updateOrder;
		};

		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;
		typedef uint32_t Entity;

		typedef std::unordered_map<std::string_view, std::set<Entity>> EntityMap;

		class System
		{
		public:

			~System()
			{
				//m_world.reset();
			}

			EntityMap entityMap;

			// Virtual functions to be called by ECSWorld
			virtual void Init() = 0;	// Called when engine starts
			virtual void Start() = 0;	// Called when gameplay starts
			virtual void Update() = 0;	// Called each frame, depending on what update order is set
			virtual void Stop() = 0;	// Called when gameplay ends
			virtual void Cleanup() = 0;	// Called when engine exits
			
			//
			virtual SystemInfo GetInfo() = 0;

			void SetWorld(std::shared_ptr<World> inWorld)
			{
				m_world = inWorld;
			}

			void SetDeltaTime(double inDeltaTime)
			{
				m_deltaTime = inDeltaTime;
			}

			void SetFixedTime(double inFixedTime)
			{
				m_fixedTime = inFixedTime;
			}

		protected:

			std::shared_ptr<World> m_world;
			double m_deltaTime; // Time it took last frame to complete
			double m_fixedTime; // Fixed time step used by deterministic physics code

		private:


		};
	}
}
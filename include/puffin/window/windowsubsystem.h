#pragma once

#include <unordered_map>

#include "puffin/types/uuid.h"
#include "puffin/core/subsystem.h"

class GLFWwindow;
class GLFWmonitor;

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystem : public core::Subsystem
		{
		public:

			WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystem_manager) override;
			void Deinitialize() override;

			GLFWwindow* primary_window() const
			{
				return m_primary_window;
			}

			[[nodiscard]] bool should_primary_window_close() const;
			GLFWmonitor* primary_monitor() const;

			// Create new window and return PuffinId handle to it
			PuffinID create_new_window(const int& width, const int& height);

			// Retrieve window using PuffinId handle
			GLFWwindow* get_window(const PuffinID& uuid);

			void destroy_window(const PuffinID& uuid);

		private:

			GLFWmonitor* m_primary_monitor = nullptr;
			GLFWwindow* m_primary_window = nullptr;

			std::unordered_map<PuffinID, GLFWwindow*> m_windows;


		};
	}
}
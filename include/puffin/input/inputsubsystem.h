#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "GLFW/glfw3.h"

#include "puffin/core/subsystem.h"
#include "puffin/input/inputevent.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace input
	{
		struct InputAction
		{
			std::string name;
			int id = 0;
			std::vector<int> keys;
			KeyState state;
		};

		class InputSubsystem : public core::Subsystem
		{
		public:

			explicit InputSubsystem(const std::shared_ptr<core::Engine>& engine);
			~InputSubsystem() override;

			void Initialize(core::SubsystemManager* subsystem_manager) override;
			void Deinitialize() override;

			core::SubsystemType GetType() const override;

			void ProcessInput() override;

			void add_action(std::string name, int key);
			void add_action(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction get_action(std::string name) const;

			[[nodiscard]] bool just_pressed(const std::string& name) const;
			[[nodiscard]] bool pressed(const std::string& name) const;
			[[nodiscard]] bool just_released(const std::string& name) const;
			[[nodiscard]] bool released(const std::string& name) const;

			[[nodiscard]] double get_mouse_x_offset() const;
			[[nodiscard]] double get_mouse_y_offset() const;
			[[nodiscard]] double sensitivity() const;
			[[nodiscard]] bool cursor_locked() const;

		private:

			double m_x_pos, m_y_pos, m_last_x_pos, m_last_y_pos;
			bool m_cursor_locked;
			double m_sensitivity;
			bool m_first_mouse;

			int m_next_id = 1;
			std::unordered_map<std::string, InputAction> m_actions;
			GLFWwindow* m_window;
		};
	}
}

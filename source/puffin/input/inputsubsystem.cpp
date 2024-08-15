#include "puffin/input/input_subsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/signal_subsystem.h"
#include "puffin/core/subsystem_manager.h"
#include "puffin/window/window_subsystem.h"

namespace puffin
{
	namespace input
	{
		InputSubsystem::InputSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			m_name = "InputSubsystem";

			m_next_id = 1;
			m_last_x_pos = 640.0;
			m_last_y_pos = 360.0;
			m_sensitivity = 0.05;
			m_cursor_locked = false;
			m_first_mouse = true;
		}

		InputSubsystem::~InputSubsystem()
		{
			m_engine = nullptr;
		}

		void InputSubsystem::initialize(core::SubsystemManager* subsystem_manager)
		{
			auto window_subsystem = subsystem_manager->create_and_initialize_subsystem<window::WindowSubsystem>();
			auto signal_subsystem = subsystem_manager->create_and_initialize_subsystem<core::SignalSubsystem>();

			m_window = window_subsystem->primary_window();

			// Setup Actions

			// Camera Actions
			add_action("EditorCamMoveForward", GLFW_KEY_W);
			add_action("EditorCamMoveBackward", GLFW_KEY_S);
			add_action("EditorCamMoveLeft", GLFW_KEY_A);
			add_action("EditorCamMoveRight", GLFW_KEY_D);
			add_action("EditorCamMoveUp", GLFW_KEY_E);
			add_action("EditorCamMoveDown", GLFW_KEY_Q);
			add_action("EditorCursorSwitch", GLFW_KEY_F1);
			//add_action("Spacebar", GLFW_KEY_SPACE);
			//add_action("Play", GLFW_KEY_P);
			//add_action("Restart", GLFW_KEY_O);

			// Setup Mouse Cursor
			if (m_cursor_locked == true)
			{
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}

		void InputSubsystem::deinitialize()
		{
			m_actions.clear();
			m_window = nullptr;
		}

		core::SubsystemType InputSubsystem::type() const
		{
			return core::SubsystemType::Input;
		}

		void InputSubsystem::process_input()
		{
			if (!m_window)
			{
				m_window = m_engine->get_subsystem<window::WindowSubsystem>()->primary_window();
			}

			glfwPollEvents();

			// Update Actions

			bool stateChanged = false;

			// Loop through current actions and update action states
			for (auto& [name, action] : m_actions)
			{
				stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
					int state = glfwGetKey(m_window, key);

					if (state == GLFW_PRESS)
					{
						if (!stateChanged && action.state == KeyState::Released)
						{
							action.state = KeyState::JustPressed;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::JustPressed)
						{
							action.state = KeyState::Pressed;
							stateChanged = true;
						}
					}

					if (state == GLFW_RELEASE)
					{
						if (!stateChanged && action.state == KeyState::Pressed)
						{
							action.state = KeyState::JustReleased;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::JustReleased)
						{
							action.state = KeyState::Released;
							stateChanged = true;
						}
					}

					// Notify subscribers that event changed
					if (stateChanged == true)
					{
						auto signalSubsystem = m_engine->get_subsystem<core::SignalSubsystem>();

						signalSubsystem->emit<InputEvent>(name, InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

			// Update Mouse
			if (get_action("EditorCursorSwitch").state == KeyState::JustPressed)
			{
				if (m_cursor_locked == true)
				{
					glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else
				{
					glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				m_cursor_locked = !m_cursor_locked;
			}

			// Update Current and Last Mouse Positions
			m_last_x_pos = m_x_pos;
			m_last_y_pos = m_y_pos;
			glfwGetCursorPos(m_window, &m_x_pos, &m_y_pos);

			// Prevent Camera Jumping when window first starts
			if (m_first_mouse)
			{
				m_last_x_pos = m_x_pos;
				m_last_y_pos = m_y_pos;
				m_first_mouse = false;
			}
		}

		void InputSubsystem::add_action(std::string name, int key)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = m_next_id;
			new_action.keys.push_back(key);
			new_action.state = KeyState::Released;

			m_actions.emplace(name, new_action);

            auto signal_subsystem = m_engine->get_subsystem<core::SignalSubsystem>();
            signal_subsystem->create_signal<InputEvent>(name);

			m_next_id++;
		}

		void InputSubsystem::add_action(std::string name, std::vector<int> keys)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = m_next_id;
			new_action.keys = keys;
			new_action.state = KeyState::Released;

			m_actions.emplace(name, new_action);

            auto signal_subsystem = m_engine->get_subsystem<core::SignalSubsystem>();
            signal_subsystem->create_signal<InputEvent>(name);

			m_next_id++;
		}

		InputAction InputSubsystem::get_action(std::string name) const
		{
			if (m_actions.find(name) != m_actions.end())
				return m_actions.at(name);

			return {};
		}

		bool InputSubsystem::just_pressed(const std::string& name) const
		{
			return m_actions.at(name).state == KeyState::JustPressed ? true : false;
		}

		bool InputSubsystem::pressed(const std::string& name) const
		{
			return m_actions.at(name).state == KeyState::Pressed ? true : false;
		}

		bool InputSubsystem::just_released(const std::string& name) const
		{
			return m_actions.at(name).state == KeyState::JustReleased ? true : false;
		}

		bool InputSubsystem::released(const std::string& name) const
		{
			return m_actions.at(name).state == KeyState::Released ? true : false;
		}

		double InputSubsystem::get_mouse_x_offset() const
		{
			return (m_x_pos - m_last_x_pos) * m_sensitivity;
		}

		double InputSubsystem::get_mouse_y_offset() const
		{
			return (m_y_pos - m_last_y_pos) * m_sensitivity;
		}

		double InputSubsystem::sensitivity() const
		{
			return m_sensitivity;
		}

		bool InputSubsystem::cursor_locked() const
		{
			return m_cursor_locked;
		}
	}
}

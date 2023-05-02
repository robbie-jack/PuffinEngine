#include <Input/InputSubsystem.h>
#include <ECS/ECS.h>

#include "Engine/Engine.hpp"
#include "Engine/EventSubsystem.hpp"
#include "Engine/SignalSubsystem.hpp"
#include "Window/WindowSubsystem.hpp"

namespace Puffin
{
	namespace Input
	{
		void InputSubsystem::SetupCallbacks()
		{
			m_engine->RegisterCallback(Core::ExecutionStage::Init, [&]() { Init(); }, "InputSubsystem: Init", 50);
			m_engine->RegisterCallback(Core::ExecutionStage::SubsystemUpdate, [&]() { Update(); }, "InputSubsystem: Update");
			m_engine->RegisterCallback(Core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "InputSubsystem: Cleanup", 150);
		}

		void InputSubsystem::Init()
		{
			m_window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();
			m_world = m_engine->GetSubsystem<ECS::World>();

			auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

			eventSubsystem->RegisterEvent<InputEvent>();

			// Setup Actions

			// Camera Actions
			AddAction("CamMoveForward", GLFW_KEY_W);
			AddAction("CamMoveBackward", GLFW_KEY_S);
			AddAction("CamMoveLeft", GLFW_KEY_A);
			AddAction("CamMoveRight", GLFW_KEY_D);
			AddAction("CamMoveUp", GLFW_KEY_E);
			AddAction("CamMoveDown", GLFW_KEY_Q);
			AddAction("CursorSwitch", GLFW_KEY_F1);
			AddAction("Spacebar", GLFW_KEY_SPACE);
			AddAction("Play", GLFW_KEY_P);
			AddAction("Restart", GLFW_KEY_O);

			// Setup Mouse Cursor
			if (cursor_locked == true)
			{
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}

		void Puffin::Input::InputSubsystem::Update()
		{
			glfwPollEvents();

			// Update Actions

			bool stateChanged = false;

			// Loop through current actions and update action states
			for (auto& action : m_actions)
			{
				stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
					int state = glfwGetKey(m_window, key);

					if (state == GLFW_PRESS)
					{
						if (!stateChanged && action.state == KeyState::UP)
						{
							action.state = KeyState::PRESSED;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::PRESSED)
						{
							action.state = KeyState::HELD;
							stateChanged = true;
						}
					}

					if (state == GLFW_RELEASE)
					{
						if (!stateChanged && action.state == KeyState::HELD)
						{
							action.state = KeyState::RELEASED;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::RELEASED)
						{
							action.state = KeyState::UP;
							stateChanged = true;
						}
					}

					// Notify subscribers that event changed
					if (stateChanged == true)
					{
						auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();
						auto signalSubsystem = m_engine->GetSubsystem<Core::SignalSubsystem>();

						eventSubsystem->Publish(InputEvent(action.name, action.state));
						signalSubsystem->Signal(InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

			// Update Mouse
			if (GetAction("CursorSwitch").state == KeyState::PRESSED)
			{
				if (cursor_locked == true)
				{
					glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else
				{
					glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				cursor_locked = !cursor_locked;
			}

			// Update Current and Last Mouse Positions
			last_x_pos = x_pos;
			last_y_pos = y_pos;
			glfwGetCursorPos(m_window, &x_pos, &y_pos);

			// Prevent Camera Jumping when window first starts
			if (firstMouse)
			{
				last_x_pos = x_pos;
				last_y_pos = y_pos;
				firstMouse = false;
			}
		}

		void InputSubsystem::Cleanup()
		{
			m_world = nullptr;
			m_window = nullptr;
		}

		void Puffin::Input::InputSubsystem::AddAction(std::string name, int key)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = nextID;
			new_action.keys.push_back(key);
			new_action.state = KeyState::UP;

			m_actions.push_back(new_action);

			nextID++;
		}

		void Puffin::Input::InputSubsystem::AddAction(std::string name, std::vector<int> keys)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = nextID;
			new_action.keys = keys;
			new_action.state = KeyState::UP;

			m_actions.push_back(new_action);

			nextID++;
		}

		InputAction Puffin::Input::InputSubsystem::GetAction(std::string name) const
		{
			for (auto action : m_actions)
			{
				if (action.name == name)
				{
					return action;
				}
			}

			return InputAction();
		}
	}
}

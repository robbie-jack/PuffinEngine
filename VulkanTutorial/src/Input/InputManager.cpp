#include "InputManager.h"

namespace Puffin
{
	namespace Input
	{
		Puffin::Input::InputManager::InputManager()
		{
			nextID = 1;
			last_x_pos = 640.0f;
			last_y_pos = 360.0f;
			sensitivity = 0.05f;
			cursor_locked = false;
			firstMouse = true;
		}

		Puffin::Input::InputManager::~InputManager()
		{

		}

		void Puffin::Input::InputManager::SetupInput(GLFWwindow* window)
		{
			// Setup Actions

			// Camera Actions
			AddAction("CamMoveForward", GLFW_KEY_W);
			AddAction("CamMoveBackward", GLFW_KEY_S);
			AddAction("CamMoveLeft", GLFW_KEY_A);
			AddAction("CamMoveRight", GLFW_KEY_D);
			AddAction("CamMoveUp", GLFW_KEY_E);
			AddAction("CamMoveDown", GLFW_KEY_Q);
			AddAction("CursorSwitch", GLFW_KEY_F1);

			// Setup Mouse Cursor
			if (cursor_locked == true)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}

		void Puffin::Input::InputManager::UpdateInput(GLFWwindow* window)
		{
			// Update Actions

			// Loop through current actions and update action states
			for (int i = 0; i < actions.size(); i++)
			{
				for (int j = 0; j < actions[i].keys.size(); j++)
				{
					//actions[i].state = glfwGetKey(window, actions[i].keys[j]);
					int state = glfwGetKey(window, actions[i].keys[j]);

					if (state == GLFW_PRESS)
					{
						if (actions[i].state == UP)
						{
							actions[i].state = PRESSED;
							break;
						}

						if (actions[i].state == PRESSED)
						{
							actions[i].state = HELD;
							break;
						}
					}

					if (state == GLFW_RELEASE)
					{
						if (actions[i].state == HELD)
						{
							actions[i].state = RELEASED;
							break;
						}

						if (actions[i].state == RELEASED)
						{
							actions[i].state = UP;
							break;
						}
					}
				}
			}

			// Update Mouse
			if (GetAction("CursorSwitch").state == PRESSED)
			{
				if (cursor_locked == true)
				{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else
				{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				cursor_locked = !cursor_locked;
			}

			// Update Current and Last Mouse Positions
			last_x_pos = x_pos;
			last_y_pos = y_pos;
			glfwGetCursorPos(window, &x_pos, &y_pos);

			// Prevent Camera Jumping when window first starts
			if (firstMouse)
			{
				last_x_pos = x_pos;
				last_y_pos = y_pos;
				firstMouse = false;
			}
		}

		void Puffin::Input::InputManager::AddAction(std::string name, int key)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = nextID;
			new_action.keys.push_back(key);
			new_action.state = UP;

			actions.push_back(new_action);

			nextID++;
		}

		void Puffin::Input::InputManager::AddAction(std::string name, std::vector<int> keys)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = nextID;
			new_action.keys = keys;
			new_action.state = UP;

			actions.push_back(new_action);

			nextID++;
		}

		Puffin::Input::InputAction Puffin::Input::InputManager::GetAction(std::string name)
		{
			for (int i = 0; i < actions.size(); i++)
			{
				if (actions[i].name == name)
				{
					return actions[i];
				}
			}
		}
	}
}
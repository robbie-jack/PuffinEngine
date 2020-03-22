#include "InputManager.h"

InputManager::InputManager()
{
	nextID = 1;
	last_x_pos = 640.0f;
	last_y_pos = 360.0f;
	sensitivity = 0.05f;
	cursor_locked = true;
}

InputManager::~InputManager()
{

}

void InputManager::UpdateInput(GLFWwindow* window)
{
	// Update Actions

	// Loop through current actions and update action states
	for (int i = 0; i < actions.size(); i++)
	{
		for (int j = 0; j < actions[i].keys.size(); j++)
		{
			actions[i].state = glfwGetKey(window, actions[i].keys[j]);
		}
	}

	// Update Mouse
	if (GetAction("CursorSwitch").state == GLFW_PRESS)
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
	
	last_x_pos = x_pos;
	last_y_pos = y_pos;
	glfwGetCursorPos(window, &x_pos, &y_pos);
}

void InputManager::AddAction(std::string name, int key)
{
	InputAction new_action;
	new_action.name = name;
	new_action.id = nextID;
	new_action.keys.push_back(key);
	new_action.state = GLFW_RELEASE;

	actions.push_back(new_action);

	nextID++;
}

void InputManager::AddAction(std::string name, std::vector<int> keys)
{
	InputAction new_action;
	new_action.name = name;
	new_action.id = nextID;
	new_action.keys = keys;
	new_action.state = GLFW_RELEASE;

	actions.push_back(new_action);

	nextID++;
}

InputAction InputManager::GetAction(std::string name)
{
	for (int i = 0; i < actions.size(); i++)
	{
		if (actions[i].name == name)
		{
			return actions[i];
		}
	}
}
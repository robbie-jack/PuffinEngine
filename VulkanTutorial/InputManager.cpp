#include "InputManager.h"

InputManager::InputManager()
{
	
}

InputManager::~InputManager()
{

}

void InputManager::Init()
{
	nextID = 1;
}

void InputManager::UpdateInput(GLFWwindow* window)
{
	// Loop through current actions and update action states
	for (int i = 0; i < actions.size(); i++)
	{
		for (int j = 0; j < actions[i].keys.size(); j++)
		{
			actions[i].state = glfwGetKey(window, actions[i].keys[j]);
		}
	}
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
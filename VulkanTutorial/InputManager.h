#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <string>

struct InputAction
{
	std::string name;
	int id;
	std::vector<int> keys;
	int state;
};

class InputManager
{
public: 
	InputManager();
	~InputManager();

	void Init();
	void UpdateInput(GLFWwindow* window);
	void AddAction(std::string name, int key);
	void AddAction(std::string name, std::vector<int> keys);
	InputAction GetAction(std::string name);

private:

	int nextID;
	std::vector<InputAction> actions;
};
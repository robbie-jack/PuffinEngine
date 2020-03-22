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

	void UpdateInput(GLFWwindow* window);
	void AddAction(std::string name, int key);
	void AddAction(std::string name, std::vector<int> keys);
	InputAction GetAction(std::string name);

	inline float GetMouseXOffset() { return (x_pos - last_x_pos) * sensitivity; };
	inline float GetMouseYOffset() { return (y_pos - last_y_pos) * sensitivity; };
	inline float& GetSensitivity() { return sensitivity; };
	inline bool IsCursorLocked() { return cursor_locked; };

private:

	double x_pos, y_pos, last_x_pos, last_y_pos;
	bool cursor_locked;
	float sensitivity;

	int nextID = 1;
	std::vector<InputAction> actions;
};
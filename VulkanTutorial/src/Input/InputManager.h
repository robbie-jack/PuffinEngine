#pragma once

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>

#include "InputEvent.h"

#include <vector>
#include <string>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		class World;
	}

	namespace Input
	{
		struct InputAction
		{
			std::string name;
			int id;
			std::vector<int> keys;
			KeyState state;
		};

		class InputManager
		{
		public:
			InputManager();
			~InputManager();

			void Init(GLFWwindow* windowIn, std::shared_ptr<ECS::World> InWorld);

			void UpdateInput();
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
			bool firstMouse;

			int nextID = 1;
			std::vector<InputAction> actions;
			GLFWwindow* window;

			std::shared_ptr<ECS::World> world;
		};
	}
}

#endif // INPUT_MANAGER_H
#pragma once

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>
//#include <Types/RingBuffer.h>

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
		enum class KeyState
		{
			PRESSED = 0,
			HELD = 1,
			RELEASED = 2,
			UP = 3
		};

		struct InputAction
		{
			std::string name;
			int id;
			std::vector<int> keys;
			KeyState state;
		};

		struct InputEvent
		{
			InputEvent(std::string name, KeyState state) : actionName{ name }, actionState{ state } {};
			std::string actionName;
			KeyState actionState;
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
			//RingBuffer<InputEvent> inputEvents;
		};
	}
}

#endif // INPUT_MANAGER_H
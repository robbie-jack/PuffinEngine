#pragma once

#include "glfw/glfw3.h"

#include "Engine/Subsystem.hpp"
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

		class InputSubsystem : public Core::Subsystem
		{
		public:

			InputSubsystem()
			{
				nextID = 1;
				last_x_pos = 640.0;
				last_y_pos = 360.0;
				sensitivity = 0.05;
				cursor_locked = false;
				firstMouse = true;
			}

			~InputSubsystem() override = default;

			void SetupCallbacks() override;

			void Init();
			void Update();
			void Cleanup();

			void AddAction(std::string name, int key);
			void AddAction(std::string name, std::vector<int> keys);
			InputAction GetAction(std::string name) const;

			inline double GetMouseXOffset() { return (x_pos - last_x_pos) * sensitivity; }
			inline double GetMouseYOffset() { return (y_pos - last_y_pos) * sensitivity; }
			inline double& GetSensitivity() { return sensitivity; }
			inline bool IsCursorLocked() { return cursor_locked; }

		private:

			double x_pos, y_pos, last_x_pos, last_y_pos;
			bool cursor_locked;
			double sensitivity;
			bool firstMouse;

			int nextID = 1;
			std::vector<InputAction> m_actions;
			GLFWwindow* m_window;

			std::shared_ptr<ECS::World> m_world;
		};
	}
}
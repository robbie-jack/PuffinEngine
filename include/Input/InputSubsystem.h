#pragma once

#include "GLFW/glfw3.h"

#include "Core/Subsystem.h"
#include "InputEvent.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace puffin
{
	namespace ecs
	{
		class World;
	}

	namespace input
	{
		struct InputAction
		{
			std::string name;
			int id = 0;
			std::vector<int> keys;
			KeyState state;
		};

		class InputSubsystem : public core::Subsystem
		{
		public:

			InputSubsystem()
			{
				mNextId = 1;
				mLastXPos = 640.0;
				mLastYPos = 360.0;
				mSensitivity = 0.05;
				mCursorLocked = false;
				mFirstMouse = true;
			}

			~InputSubsystem() override = default;

			void setup() override;

			void init();
			void update();
			void shutdown();

			void addAction(std::string name, int key);
			void addAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction getAction(std::string name) const;

			bool justPressed(const std::string& name) const;
			bool pressed(const std::string& name) const;
			bool justReleased(const std::string& name) const;
			bool released(const std::string& name) const;

			double getMouseXOffset() const { return (mXPos - mLastXPos) * mSensitivity; }
			double getMouseYOffset() const { return (mYPos - mLastYPos) * mSensitivity; }
			double& sensitivity() { return mSensitivity; }
			bool isCursorLocked() const { return mCursorLocked; }

		private:

			double mXPos, mYPos, mLastXPos, mLastYPos;
			bool mCursorLocked;
			double mSensitivity;
			bool mFirstMouse;

			int mNextId = 1;
			std::unordered_map<std::string, InputAction> mActions;
			GLFWwindow* mWindow;
		};
	}
}

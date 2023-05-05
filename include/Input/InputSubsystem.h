#pragma once

#include "glfw/glfw3.h"

#include "Engine/Subsystem.hpp"
#include "InputEvent.h"

#include <vector>
#include <string>
#include <memory>

namespace puffin
{
	namespace ECS
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

			void setupCallbacks() override;

			void init();
			void update();
			void cleanup();

			void addAction(std::string name, int key);
			void addAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction getAction(std::string name) const;

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
			std::vector<InputAction> mActions;
			GLFWwindow* mWindow;
		};
	}
}
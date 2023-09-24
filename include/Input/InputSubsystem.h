#pragma once

#include "GLFW/glfw3.h"

#include "Core/Subsystem.h"
#include "InputEvent.h"

#include <vector>
#include <string>
#include <memory>
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

			void setupCallbacks() override;

			void init();
			void update();
			void shutdown();

			void addAction(std::string name, int key);
			void addAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction getAction(std::string name) const;

			bool isJustPressed(const std::string& name) const;
			bool isPressed(const std::string& name) const;
			bool isJustReleased(const std::string& name) const;
			bool isReleased(const std::string& name) const;

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

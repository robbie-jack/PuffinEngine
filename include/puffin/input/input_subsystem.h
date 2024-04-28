#pragma once

#include "GLFW/glfw3.h"

#include <vector>
#include <string>
#include <unordered_map>

#include "puffin/core/system.h"
#include "puffin/input/input_event.h"

namespace puffin
{
	namespace core
	{
		class Engine;
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

		class InputSubsystem : public core::System
		{
		public:

			InputSubsystem(const std::shared_ptr<core::Engine>& engine);

			~InputSubsystem() override { mEngine = nullptr; }

			void startup();
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

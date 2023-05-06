#include "Input/InputSubsystem.h"

#include "Core/Engine.h"
#include "Core/SignalSubsystem.h"
#include "Window/WindowSubsystem.h"

namespace puffin
{
	namespace input
	{
		void InputSubsystem::setupCallbacks()
		{
			mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "InputSubsystem: Init", 50);
			mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "InputSubsystem: Update");
			mEngine->registerCallback(core::ExecutionStage::Cleanup, [&]() { cleanup(); }, "InputSubsystem: Cleanup", 150);
		}

		void InputSubsystem::init()
		{
			mWindow = mEngine->getSubsystem<Window::WindowSubsystem>()->primaryWindow();

			// Setup Actions

			// Camera Actions
			addAction("CamMoveForward", GLFW_KEY_W);
			addAction("CamMoveBackward", GLFW_KEY_S);
			addAction("CamMoveLeft", GLFW_KEY_A);
			addAction("CamMoveRight", GLFW_KEY_D);
			addAction("CamMoveUp", GLFW_KEY_E);
			addAction("CamMoveDown", GLFW_KEY_Q);
			addAction("CursorSwitch", GLFW_KEY_F1);
			addAction("Spacebar", GLFW_KEY_SPACE);
			addAction("Play", GLFW_KEY_P);
			addAction("Restart", GLFW_KEY_O);

			// Setup Mouse Cursor
			if (mCursorLocked == true)
			{
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}

		void puffin::input::InputSubsystem::update()
		{
			glfwPollEvents();

			// Update Actions

			bool stateChanged = false;

			// Loop through current actions and update action states
			for (auto& action : mActions)
			{
				stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
					int state = glfwGetKey(mWindow, key);

					if (state == GLFW_PRESS)
					{
						if (!stateChanged && action.state == KeyState::Up)
						{
							action.state = KeyState::Pressed;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::Pressed)
						{
							action.state = KeyState::Held;
							stateChanged = true;
						}
					}

					if (state == GLFW_RELEASE)
					{
						if (!stateChanged && action.state == KeyState::Held)
						{
							action.state = KeyState::Released;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::Released)
						{
							action.state = KeyState::Up;
							stateChanged = true;
						}
					}

					// Notify subscribers that event changed
					if (stateChanged == true)
					{
						auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

						signalSubsystem->signal(InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

			// Update Mouse
			if (getAction("CursorSwitch").state == KeyState::Pressed)
			{
				if (mCursorLocked == true)
				{
					glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else
				{
					glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				mCursorLocked = !mCursorLocked;
			}

			// Update Current and Last Mouse Positions
			mLastXPos = mXPos;
			mLastYPos = mYPos;
			glfwGetCursorPos(mWindow, &mXPos, &mYPos);

			// Prevent Camera Jumping when window first starts
			if (mFirstMouse)
			{
				mLastXPos = mXPos;
				mLastYPos = mYPos;
				mFirstMouse = false;
			}
		}

		void InputSubsystem::cleanup()
		{
			mWindow = nullptr;
		}

		void puffin::input::InputSubsystem::addAction(std::string name, int key)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = mNextId;
			new_action.keys.push_back(key);
			new_action.state = KeyState::Up;

			mActions.push_back(new_action);

			mNextId++;
		}

		void puffin::input::InputSubsystem::addAction(std::string name, std::vector<int> keys)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = mNextId;
			new_action.keys = keys;
			new_action.state = KeyState::Up;

			mActions.push_back(new_action);

			mNextId++;
		}

		InputAction puffin::input::InputSubsystem::getAction(std::string name) const
		{
			for (auto action : mActions)
			{
				if (action.name == name)
				{
					return action;
				}
			}

			return InputAction();
		}
	}
}

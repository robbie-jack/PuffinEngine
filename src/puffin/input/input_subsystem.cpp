#include "puffin/input/input_subsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/signal_subsystem.h"
#include "puffin/window/window_subsystem.h"

namespace puffin
{
	namespace input
	{
		InputSubsystem::InputSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			mEngine->registerCallback(core::ExecutionStage::Startup, [&]() { startup(); }, "InputSubsystem: Startup", 50);
			mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "InputSubsystem: Update");
			mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "InputSubsystem: Shutdown", 150);

			mNextId = 1;
			mLastXPos = 640.0;
			mLastYPos = 360.0;
			mSensitivity = 0.05;
			mCursorLocked = false;
			mFirstMouse = true;
		}

		void InputSubsystem::startup()
		{
			mWindow = mEngine->getSystem<window::WindowSubsystem>()->primaryWindow();

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
			//addAction("Play", GLFW_KEY_P);
			//addAction("Restart", GLFW_KEY_O);

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

		void InputSubsystem::update()
		{
			glfwPollEvents();

			// Update Actions

			bool stateChanged = false;

			// Loop through current actions and update action states
			for (auto& [name, action] : mActions)
			{
				stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
					int state = glfwGetKey(mWindow, key);

					if (state == GLFW_PRESS)
					{
						if (!stateChanged && action.state == KeyState::Released)
						{
							action.state = KeyState::JustPressed;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::JustPressed)
						{
							action.state = KeyState::Pressed;
							stateChanged = true;
						}
					}

					if (state == GLFW_RELEASE)
					{
						if (!stateChanged && action.state == KeyState::Pressed)
						{
							action.state = KeyState::JustReleased;
							stateChanged = true;
						}

						if (!stateChanged && action.state == KeyState::JustReleased)
						{
							action.state = KeyState::Released;
							stateChanged = true;
						}
					}

					// Notify subscribers that event changed
					if (stateChanged == true)
					{
						auto signalSubsystem = mEngine->getSystem<core::SignalSubsystem>();

						signalSubsystem->emit<InputEvent>(name, InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

			// Update Mouse
			if (getAction("CursorSwitch").state == KeyState::JustPressed)
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

		void InputSubsystem::shutdown()
		{
			mWindow = nullptr;
		}

		void InputSubsystem::addAction(std::string name, int key)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = mNextId;
			new_action.keys.push_back(key);
			new_action.state = KeyState::Released;

			mActions.emplace(name, new_action);

			mNextId++;
		}

		void InputSubsystem::addAction(std::string name, std::vector<int> keys)
		{
			InputAction new_action;
			new_action.name = name;
			new_action.id = mNextId;
			new_action.keys = keys;
			new_action.state = KeyState::Released;

			mActions.emplace(name, new_action);

			mNextId++;
		}

		InputAction InputSubsystem::getAction(std::string name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name);

			return InputAction();
		}

		bool InputSubsystem::justPressed(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::JustPressed ? true : false;
		}

		bool InputSubsystem::pressed(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Pressed ? true : false;
		}

		bool InputSubsystem::justReleased(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::JustReleased ? true : false;
		}

		bool InputSubsystem::released(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Released ? true : false;
		}
	}
}

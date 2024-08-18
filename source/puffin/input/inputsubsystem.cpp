#include "puffin/input/inputsubsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/core/subsystemmanager.h"
#include "puffin/window/windowsubsystem.h"

namespace puffin
{
	namespace input
	{
		InputSubsystem::InputSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			mName = "InputSubsystem";

			mNextID = 1;
			mLastXPos = 640.0;
			mLastYPos = 360.0;
			m_sensitivity = 0.05;
			mCursorLocked = false;
			mFirstMouse = true;
		}

		void InputSubsystem::Initialize(core::SubsystemManager* subsystemManager)
		{
			const auto windowSubsystem = subsystemManager->CreateAndInitializeSubsystem<window::WindowSubsystem>();

			mWindow = windowSubsystem->primary_window();

			// Setup Actions

			// Camera Actions
			AddAction("EditorCamMoveForward", GLFW_KEY_W);
			AddAction("EditorCamMoveBackward", GLFW_KEY_S);
			AddAction("EditorCamMoveLeft", GLFW_KEY_A);
			AddAction("EditorCamMoveRight", GLFW_KEY_D);
			AddAction("EditorCamMoveUp", GLFW_KEY_E);
			AddAction("EditorCamMoveDown", GLFW_KEY_Q);
			AddAction("EditorCursorSwitch", GLFW_KEY_F1);
			//add_action("Spacebar", GLFW_KEY_SPACE);
			//add_action("Play", GLFW_KEY_P);
			//add_action("Restart", GLFW_KEY_O);

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

		void InputSubsystem::Deinitialize()
		{
			mActions.clear();
			mWindow = nullptr;
		}

		core::SubsystemType InputSubsystem::GetType() const
		{
			return core::SubsystemType::Input;
		}

		void InputSubsystem::ProcessInput()
		{
			if (!mWindow)
			{
				mWindow = mEngine->GetSubsystem<window::WindowSubsystem>()->primary_window();
			}

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
						const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

						signalSubsystem->Emit<InputEvent>(name, InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

			// Update Mouse
			if (GetAction("EditorCursorSwitch").state == KeyState::JustPressed)
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

		void InputSubsystem::AddAction(std::string name, int key)
		{
			InputAction newAction;
			newAction.name = name;
			newAction.id = mNextID;
			newAction.keys.push_back(key);
			newAction.state = KeyState::Released;

			mActions.emplace(name, newAction);

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);

			mNextID++;
		}

		void InputSubsystem::AddAction(std::string name, std::vector<int> keys)
		{
			InputAction newAction;
			newAction.name = name;
			newAction.id = mNextID;
			newAction.keys = keys;
			newAction.state = KeyState::Released;

			mActions.emplace(name, newAction);

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);

			mNextID++;
		}

		InputAction InputSubsystem::GetAction(std::string name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name);

			return {};
		}

		bool InputSubsystem::JustPressed(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::JustPressed ? true : false;
		}

		bool InputSubsystem::Pressed(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Pressed ? true : false;
		}

		bool InputSubsystem::JustReleased(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::JustReleased ? true : false;
		}

		bool InputSubsystem::Released(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Released ? true : false;
		}

		double InputSubsystem::GetMouseXOffset() const
		{
			return (mXPos - mLastXPos) * m_sensitivity;
		}

		double InputSubsystem::GetMouseYOffset() const
		{
			return (mYPos - mLastYPos) * m_sensitivity;
		}

		double InputSubsystem::GetSensitivity() const
		{
			return m_sensitivity;
		}

		bool InputSubsystem::GetCursorLocked() const
		{
			return mCursorLocked;
		}
	}
}

#include <utility>

#include "puffin/input/inputsubsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"
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
			mSensitivity = 0.05;
			mCursorLocked = false;
			mFirstMouse = true;
		}

		void InputSubsystem::Initialize(core::SubsystemManager* subsystemManager)
		{
			//const auto windowSubsystem = subsystemManager->CreateAndInitializeSubsystem<window::WindowSubsystem>();
			//const auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
			//const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();
			//
			////mWindow = windowSubsystem->GetPrimaryWindow();
			//mSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05);

			//// Setup Actions

			//// Camera Actions
			

			//// Setup Mouse Cursor
			//if (mCursorLocked == true)
			//{
			//	//glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			//}
			//else
			//{
			//	//glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			//}

			//auto mouseSensitivitySignal = signalSubsystem->GetSignal("general_mouse_sensitivity");
			//if (!mouseSensitivitySignal)
			//{
			//	mouseSensitivitySignal = signalSubsystem->CreateSignal("general_mouse_sensitivity");
			//}

			//mouseSensitivitySignal->Connect(std::function([&]
			//{
			//	auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
			//	
			//	mSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05);
			//}));
		}

		void InputSubsystem::Deinitialize()
		{
			mActions.clear();
			//mWindow = nullptr;
		}

		core::SubsystemType InputSubsystem::GetType() const
		{
			return core::SubsystemType::Input;
		}

		void InputSubsystem::ProcessInput()
		{
			PollInput();

			// Update Actions

			bool stateChanged = false;

			// Loop through current actions and update action states
			for (auto& [name, action] : mActions)
			{
				stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
		//			/*int state = glfwGetKey(mWindow, key);

		//			if (state == GLFW_PRESS)
		//			{
		//				if (!stateChanged && action.state == KeyState::Up)
		//				{
		//					action.state = KeyState::IsActionDown;
		//					stateChanged = true;
		//				}

		//				if (!stateChanged && action.state == KeyState::IsActionDown)
		//				{
		//					action.state = KeyState::Down;
		//					stateChanged = true;
		//				}
		//			}

		//			if (state == GLFW_RELEASE)
		//			{
		//				if (!stateChanged && action.state == KeyState::Down)
		//				{
		//					action.state = KeyState::IsActionUp;
		//					stateChanged = true;
		//				}

		//				if (!stateChanged && action.state == KeyState::IsActionUp)
		//				{
		//					action.state = KeyState::Up;
		//					stateChanged = true;
		//				}
		//			}*/

					// Notify subscribers that event changed
					if (stateChanged == true)
					{
						const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

						signalSubsystem->Emit<InputEvent>(name, InputEvent(action.name, action.state));

						stateChanged = false;
					}
				}
			}

		//	// Update Mouse
		//	if (GetAction("EditorCursorSwitch").state == KeyState::IsActionDown)
		//	{
		//		if (mCursorLocked == true)
		//		{
		//			//glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//		}
		//		else
		//		{
		//			//glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		//		}

		//		mCursorLocked = !mCursorLocked;
		//	}

			// Update Current and Last Mouse Positions
			mLastXPos = mXPos;
			mLastYPos = mYPos;
			//glfwGetCursorPos(mWindow, &mXPos, &mYPos);

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
			newAction.state = KeyState::Up;

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
			newAction.keys = std::move(keys);
			newAction.state = KeyState::Up;

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

		bool InputSubsystem::IsActionPressed(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Pressed;
		}

		bool InputSubsystem::IsActionDown(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Down;
		}

		bool InputSubsystem::IsActionReleased(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Released;
		}

		bool InputSubsystem::IsActionUp(const std::string& name) const
		{
			return mActions.at(name).state == KeyState::Up;
		}

		double InputSubsystem::GetMouseXOffset() const
		{
			return (mXPos - mLastXPos) * mSensitivity;
		}

		double InputSubsystem::GetMouseYOffset() const
		{
			return (mYPos - mLastYPos) * mSensitivity;
		}

		double InputSubsystem::GetSensitivity() const
		{
			return mSensitivity;
		}

		bool InputSubsystem::GetCursorLocked() const
		{
			return mCursorLocked;
		}

		void AddEditorActions()
		{

		}
	}
}

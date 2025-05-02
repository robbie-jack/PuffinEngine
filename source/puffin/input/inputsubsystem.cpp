#include <utility>

#include "puffin/input/inputsubsystem.h"

#include "raylib.h"
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

			AddEditorContext();
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

			// Loop through current actions and publish input events
			for (auto& [name, action] : mActions)
			{
				bool stateChanged = false;

				// Loop over each key in this action
				for (auto key : action.keys)
				{
					switch (action.state)
					{
					case InputState::Up:

						if (IsKeyPressed(key.key) && AreKeyModifiersPressed(key))
						{


							stateChanged = true;
						}

						break;

					case InputState::Pressed:



						break;

					case InputState::Down:



						break;

					case InputState::Released:



						break;

					}

					

		//			/*int state = glfwGetKey(mWindow, key);

		//			if (state == GLFW_PRESS)
		//			{
		//				if (!stateChanged && action.state == InputState::Up)
		//				{
		//					action.state = InputState::IsActionDown;
		//					stateChanged = true;
		//				}

		//				if (!stateChanged && action.state == InputState::IsActionDown)
		//				{
		//					action.state = InputState::Down;
		//					stateChanged = true;
		//				}
		//			}

		//			if (state == GLFW_RELEASE)
		//			{
		//				if (!stateChanged && action.state == InputState::Down)
		//				{
		//					action.state = InputState::IsActionUp;
		//					stateChanged = true;
		//				}

		//				if (!stateChanged && action.state == InputState::IsActionUp)
		//				{
		//					action.state = InputState::Up;
		//					stateChanged = true;
		//				}
		//			}*/
				}

				// Notify subscribers that event changed
				if (stateChanged == true)
				{
					const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

					signalSubsystem->Emit<InputEvent>(name, InputEvent(action.name, action.state));

					stateChanged = false;
				}
			}

		//	// Update Mouse
		//	if (GetAction("EditorCursorSwitch").state == InputState::IsActionDown)
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

			mActions.emplace(name, newAction);

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);
		}

		void InputSubsystem::AddAction(std::string name, std::vector<int> keys)
		{
			InputAction newAction;
			newAction.name = name;

			mActions.emplace(name, newAction);

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);
		}

		InputAction InputSubsystem::GetAction(std::string name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name);

			return {};
		}

		bool InputSubsystem::IsActionPressed(const std::string& name) const
		{
			return mActions.at(name).state == InputState::Pressed;
		}

		bool InputSubsystem::IsActionDown(const std::string& name) const
		{
			return mActions.at(name).state == InputState::Down;
		}

		bool InputSubsystem::IsActionReleased(const std::string& name) const
		{
			return mActions.at(name).state == InputState::Released;
		}

		bool InputSubsystem::IsActionUp(const std::string& name) const
		{
			return mActions.at(name).state == InputState::Up;
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

		bool InputSubsystem::AreKeyModifiersPressed(KeyboardKeyWithModifier keyWithModifier) const
		{
			if (keyWithModifier.ctrlPressed && (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
				|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl)))
				return false;

			if (keyWithModifier.altPressed && (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
				|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt)))
				return false;

			if (keyWithModifier.shiftPressed && (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
				|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift)))
				return false;

			return true;
		}

		bool InputSubsystem::AreMouseModifiersPressed(MouseButtonWithModifier mouseButtonWithModifier) const
		{
			if (mouseButtonWithModifier.ctrlPressed && (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
				|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl)))
				return false;

			if (mouseButtonWithModifier.altPressed && (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
				|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt)))
				return false;

			if (mouseButtonWithModifier.shiftPressed && (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
				|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift)))
				return false;

			return true;
		}

		void AddEditorContext()
		{

		}
	}
}

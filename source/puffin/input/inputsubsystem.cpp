#include <utility>

#include "puffin/input/inputsubsystem.h"

#include <stdbool.h>

#include "raylib.h"
#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/input/inputcontext.h"

namespace puffin
{
	namespace input
	{
		InputSubsystem::InputSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			mName = "InputSubsystem";
			mMouseSensitivity = 0.05;
		}

		void InputSubsystem::Initialize(core::SubsystemManager* subsystemManager)
		{
			const auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
			const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

			InitSettings();

			AddEditorContext(this);
		}

		void InputSubsystem::Deinitialize()
		{
			mActions.clear();
		}

		core::SubsystemType InputSubsystem::GetType() const
		{
			return core::SubsystemType::Input;
		}

		void InputSubsystem::ProcessInput()
		{
			PollInput();

			// Update Actions

			// Loop through global actions and publish input events
			for (auto& [name, action] : mActions)
			{
				UpdateAction(action);
			}

			// Loop context actions and publish input events
			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				for (auto& [name, action] : context->GetActions())
				{
					UpdateAction(action);
				}

				if (context->GetBlockInput())
					break;

				++it;
			}
		}

		void InputSubsystem::AddAction(std::string name, int key)
		{
			mActions.emplace(name, InputAction(name));

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);
		}

		void InputSubsystem::AddAction(std::string name, std::vector<int> keys)
		{
			mActions.emplace(name, InputAction(name));

			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
            signalSubsystem->CreateSignal<InputEvent>(name);
		}

		InputAction InputSubsystem::GetAction(std::string name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name);

			return InputAction(name);
		}

		bool InputSubsystem::HasAction(const std::string& name) const
		{
			if (mActions.find(name) != mActions.end())
				return true;

			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				if (context->HasAction(name))
					return true;

				if (context->GetBlockInput())
					break;

				++it;
			}

			return false;
		}

		bool InputSubsystem::IsActionPressed(const std::string& name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name).state == InputState::Pressed;

			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				if (context->IsActionPressed(name))
					return true;

				if (context->GetBlockInput())
					break;

				++it;
			}

			return false;
		}

		bool InputSubsystem::IsActionDown(const std::string& name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name).state == InputState::Down;

			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				if (context->IsActionDown(name))
					return true;

				if (context->GetBlockInput())
					break;

				++it;
			}

			return false;
		}

		bool InputSubsystem::IsActionReleased(const std::string& name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name).state == InputState::Released;

			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				if (context->IsActionReleased(name))
					return true;

				if (context->GetBlockInput())
					break;

				++it;
			}

			return false;
		}

		bool InputSubsystem::IsActionUp(const std::string& name) const
		{
			if (mActions.find(name) != mActions.end())
				return mActions.at(name).state == InputState::Up;

			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				if (context->IsActionUp(name))
					return true;

				if (context->GetBlockInput())
					break;

				++it;
			}

			return false;
		}

		float InputSubsystem::GetSensitivity() const
		{
			return mMouseSensitivity;
		}

		bool InputSubsystem::AreKeyModifiersPressed(KeyboardKeyWithModifier keyWithModifier) const
		{
			if (keyWithModifier.ctrlPressed && (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
				|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl)))
				return true;

			if (keyWithModifier.altPressed && (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
				|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt)))
				return true;

			if (keyWithModifier.shiftPressed && (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
				|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift)))
				return true;

			return false;
		}

		bool InputSubsystem::AreMouseModifiersPressed(MouseButtonWithModifier mouseButtonWithModifier) const
		{
			if (mouseButtonWithModifier.ctrlPressed && (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
				|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl)))
				return true;

			if (mouseButtonWithModifier.altPressed && (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
				|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt)))
				return true;

			if (mouseButtonWithModifier.shiftPressed && (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
				|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift)))
				return true;

			return false;
		}

		InputContext* InputSubsystem::AddContext(const std::string& name)
		{
			assert(mContexts.find(name) == mContexts.end() && "InputSubsystem::AddContext - Context with name already exists");

			mContexts.emplace(name, new InputContext(name));
			mManageContextLifetime.emplace(name, true);
			mContextNamesInOrder.push_back(name);

			return mContexts.at(name);
		}

		void InputSubsystem::AddContext(const std::string& name, InputContext* context)
		{
			assert(mContexts.find(name) == mContexts.end() && "InputSubsystem::AddContext - Context with name already exists");

			mContexts.emplace(name, context);
			mManageContextLifetime.emplace(name, false);
			mContextNamesInOrder.push_back(name);
		}

		void InputSubsystem::RemoveContext(const std::string& name)
		{
			assert(mContexts.find(name) != mContexts.end() && "InputSubsystem::RemoveContext - Context with that name doesn't exist");

			if (mManageContextLifetime.at(name))
			{
				const auto context = mContexts.at(name);
				delete context;
			}

			for (auto it = mContextNamesInOrder.begin(); it != mContextNamesInOrder.end();)
			{
				if (*it == name)
				{
					mContextNamesInOrder.erase(it);
					break;
				}

				++it;
			}
		}

		InputContext* InputSubsystem::GetContext(const std::string& name)
		{
			assert(mContexts.find(name) != mContexts.end() && "InputSubsystem::GetContext - Context with that name doesn't exist");

			return mContexts.at(name);
		}

		void InputSubsystem::UpdateAction(InputAction& action)
		{
			// Loop over each keyboard key in this action
			for (const auto& key : action.keys)
			{
				switch (action.state)
				{
				case InputState::Up:

					if (IsKeyPressed(key.key) && AreKeyModifiersPressed(key))
					{
						action.state = InputState::Pressed;
					}

					break;

				case InputState::Pressed:

					if (IsKeyDown(key.key) && AreKeyModifiersPressed(key))
					{
						action.state = InputState::Down;
					}

					break;

				case InputState::Down:

					if (IsKeyReleased(key.key))
					{
						action.state = InputState::Released;
					}

					break;

				case InputState::Released:

					if (IsKeyUp(key.key))
					{
						action.state = InputState::Up;
					}

					break;

				}
			}

			// Loop over each mouse button in this action
			for (const auto& button : action.mouseButtons)
			{
				switch (action.state)
				{
				case InputState::Up:

					if (IsMouseButtonPressed(button.button) && AreMouseModifiersPressed(button))
					{
						action.state = InputState::Pressed;
					}

					break;

				case InputState::Pressed:

					if (IsMouseButtonDown(button.button) && AreMouseModifiersPressed(button))
					{
						action.state = InputState::Down;
					}

					break;

				case InputState::Down:

					if (IsMouseButtonReleased(button.button))
					{
						action.state = InputState::Released;
					}

					break;

				case InputState::Released:

					if (IsMouseButtonUp(button.button))
					{
						action.state = InputState::Up;
					}

					break;

				}
			}

			// Loop over each gamepad button in this action
			for (const auto& button : action.gamepadButtons)
			{
				switch (action.state)
				{
				case InputState::Up:

					if (IsGamepadButtonPressed(button))
					{
						action.state = InputState::Pressed;
					}

					break;

				case InputState::Pressed:

					if (IsGamepadButtonDown(button))
					{
						action.state = InputState::Down;
					}

					break;

				case InputState::Down:

					if (IsGamepadButtonReleased(button))
					{
						action.state = InputState::Released;
					}

					break;

				case InputState::Released:

					if (IsGamepadButtonUp(button))
					{
						action.state = InputState::Up;
					}

					break;

				}
			}

			// Notify subscribers that event changed
			if (action.state != action.lastState)
			{
				const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

				signalSubsystem->Emit<InputEvent>(action.name, InputEvent(action.name, action.state));

				action.lastState = action.state;
			}
		}

		void InputSubsystem::InitSettings()
		{
			const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
			const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

			mMouseSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05);

			auto mouseSensitivitySignal = signalSubsystem->GetSignal("general_mouse_sensitivity");
			if (!mouseSensitivitySignal)
			{
				mouseSensitivitySignal = signalSubsystem->CreateSignal("general_mouse_sensitivity");
			}

			mouseSensitivitySignal->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mMouseSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05);
			}));
		}

		void AddEditorContext(InputSubsystem* subsystem)
		{
			auto editorContext = subsystem->AddContext("Editor");

			auto& editorCamMoveForwardAction = editorContext->AddAction("editor_cam_move_forward");
			editorCamMoveForwardAction.keys.emplace_back(KeyboardKey::W);

			auto& editorCamMoveBackwardAction = editorContext->AddAction("editor_cam_move_backward");
			editorCamMoveBackwardAction.keys.emplace_back(KeyboardKey::S);

			auto& editorCamMoveLeftAction = editorContext->AddAction("editor_cam_move_left");
			editorCamMoveLeftAction.keys.emplace_back(KeyboardKey::A);

			auto& editorCamMoveRightAction = editorContext->AddAction("editor_cam_move_right");
			editorCamMoveRightAction.keys.emplace_back(KeyboardKey::D);

			auto& editorCamMoveUpAction = editorContext->AddAction("editor_cam_move_up");
			editorCamMoveUpAction.keys.emplace_back(KeyboardKey::E);

			auto& editorCamMoveDownAction = editorContext->AddAction("editor_cam_move_down");
			editorCamMoveDownAction.keys.emplace_back(KeyboardKey::Q);

			auto& editorCamLookAroundAction = editorContext->AddAction("editor_cam_look_around");
			editorCamLookAroundAction.mouseButtons.emplace_back(MouseButton::Right);

			auto& editorPlayPause = editorContext->AddAction("editor_play_pause");
			editorPlayPause.keys.emplace_back(KeyboardKey::P, true);

			auto& editorRestart = editorContext->AddAction("editor_restart");
			editorRestart.keys.emplace_back(KeyboardKey::O, true);
		}
	}
}

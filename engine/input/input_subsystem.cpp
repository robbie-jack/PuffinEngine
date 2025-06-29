#include "input/input_subsystem.h"

#include <utility>

#include "raylib.h"
#include "core/engine.h"
#include "core/settings_manager.h"
#include "core/signal_subsystem.h"
#include "input/input_context.h"

namespace puffin
{
	namespace input
	{
		InputSubsystem::InputSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			mName = "InputSubsystem";
			mMouseSensitivity = 0.05f;
		}

		void InputSubsystem::Initialize()
		{
			const auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
			const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

			InitSettings();
		}

		void InputSubsystem::Deinitialize()
		{
			mActions.clear();

			for (auto& [name, signal] : mActionSignals)
			{
				delete signal;
			}

			mActionSignals.clear();

			for (auto& [name, context] : mContexts)
			{
				if (mManageContextLifetime.find(name) == mManageContextLifetime.end())
					continue;

				delete context;
			}
				
			mContexts.clear();
			mManageContextLifetime.clear();
			mContextNamesInOrder.clear();
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
				UpdateAction(action, mActionSignals.at(name));
			}

			// Loop context actions and publish input events
			for (auto it = mContextNamesInOrder.rbegin(); it != mContextNamesInOrder.rend();)
			{
				auto context = mContexts.at(*it);

				for (auto& [name, action] : context->GetActions())
				{
					UpdateAction(action, context->GetActionSignal(name));
				}

				if (context->GetBlockInput())
					break;

				++it;
			}
		}

		void InputSubsystem::AddAction(std::string name, int key)
		{
			mActions.emplace(name, InputAction(name));
			mActionSignals.emplace(name, new Signal<InputEvent>());
		}

		void InputSubsystem::AddAction(std::string name, std::vector<int> keys)
		{
			mActions.emplace(name, InputAction(name));
			mActionSignals.emplace(name, new Signal<InputEvent>());
		}

		InputAction InputSubsystem::GetAction(const std::string& name) const
		{
			if (mActions.find(name) == mActions.end())
				return InputAction(name);
			
			return mActions.at(name);
		}

		Signal<InputEvent>* InputSubsystem::GetActionSignal(const std::string& name)
		{
			if (mActionSignals.find(name) == mActionSignals.end())
				return nullptr;

			return mActionSignals.at(name);
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
			if (keyWithModifier.ctrlPressed)
			{
				if (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
					|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl))
					return true;

				return false;
			}

			if (keyWithModifier.altPressed)
			{

				if (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
					|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt))
					return true;

				return false;
			}

			if (keyWithModifier.shiftPressed)
			{
				if (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
					|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift))
					return true;

				return false;
			}

			return true;
		}

		bool InputSubsystem::AreMouseModifiersPressed(MouseButtonWithModifier mouseButtonWithModifier) const
		{
			if (mouseButtonWithModifier.ctrlPressed)
			{
				if (IsKeyPressed(KeyboardKey::LeftControl) || IsKeyDown(KeyboardKey::LeftControl)
					|| IsKeyPressed(KeyboardKey::RightControl) || IsKeyDown(KeyboardKey::RightControl))
					return true;

				return false;
			}

			if (mouseButtonWithModifier.altPressed)
			{

				if (IsKeyPressed(KeyboardKey::LeftAlt) || IsKeyDown(KeyboardKey::LeftAlt)
					|| IsKeyPressed(KeyboardKey::RightAlt) || IsKeyDown(KeyboardKey::RightAlt))
					return true;

				return false;
			}

			if (mouseButtonWithModifier.shiftPressed)
			{
				if (IsKeyPressed(KeyboardKey::LeftShift) || IsKeyDown(KeyboardKey::LeftShift)
					|| IsKeyPressed(KeyboardKey::RightShift) || IsKeyDown(KeyboardKey::RightShift))
					return true;

				return false;
			}

			return true;
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

			mManageContextLifetime.erase(name);

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

		void InputSubsystem::UpdateAction(InputAction& action, Signal<InputEvent>* signal)
		{
			// Loop over each keyboard key in this action
			for (const auto& key : action.keys)
			{
				if (IsKeyPressed(key.key) && AreKeyModifiersPressed(key))
				{
					action.state = InputState::Pressed;
					continue;
				}

				if (IsKeyDown(key.key) && AreKeyModifiersPressed(key))
				{
					action.state = InputState::Down;
					continue;
				}

				if (IsKeyReleased(key.key))
				{
					action.state = InputState::Released;
					continue;
				}

				if (IsKeyUp(key.key))
				{
					action.state = InputState::Up;
				}
			}

			// Loop over each mouse button in this action
			for (const auto& button : action.mouseButtons)
			{
				if (IsMouseButtonPressed(button.button) && AreMouseModifiersPressed(button))
				{
					action.state = InputState::Pressed;
					continue;
				}

				if (IsMouseButtonDown(button.button) && AreMouseModifiersPressed(button))
				{
					action.state = InputState::Down;
					continue;
				}

				if (IsMouseButtonReleased(button.button))
				{
					action.state = InputState::Released;
					continue;
				}

				if (IsMouseButtonUp(button.button))
				{
					action.state = InputState::Up;
				}
			}

			// Loop over each gamepad button in this action
			for (const auto& button : action.gamepadButtons)
			{
				if (IsGamepadButtonPressed(button))
				{
					action.state = InputState::Pressed;
					continue;
				}

				if (IsGamepadButtonDown(button))
				{
					action.state = InputState::Down;
					continue;
				}

				if (IsGamepadButtonReleased(button))
				{
					action.state = InputState::Released;
					continue;
				}

				if (IsGamepadButtonUp(button))
				{
					action.state = InputState::Up;
				}
			}

			// Notify subscribers that event changed
			if (action.state != action.lastState && signal)
			{
				signal->Emit({ action.name, action.state });

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
	}
}

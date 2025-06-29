#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "core/subsystem.h"
#include "input/input_types.h"
#include "input/input_event.h"
#include "core/signal.h"
#include "types/vector2.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace input
	{
		class InputContext;

		class InputSubsystem : public core::Subsystem
		{
		public:

			explicit InputSubsystem(const std::shared_ptr<core::Engine>& engine);
			~InputSubsystem() override = default;

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			/*
			 * Called once a frame on input subsystem
			 */
			void ProcessInput();

			void AddAction(std::string name, int key);
			void AddAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction GetAction(const std::string& name) const;
			[[nodiscard]] Signal<InputEvent>* GetActionSignal(const std::string& name);

			[[nodiscard]] bool HasAction(const std::string& name) const;
			[[nodiscard]] bool IsActionPressed(const std::string& name) const;
			[[nodiscard]] bool IsActionDown(const std::string& name) const;
			[[nodiscard]] bool IsActionReleased(const std::string& name) const;
			[[nodiscard]] bool IsActionUp(const std::string& name) const;

			[[nodiscard]] virtual bool IsKeyPressed(KeyboardKey key) const = 0;
			[[nodiscard]] virtual bool IsKeyDown(KeyboardKey key) const = 0;
			[[nodiscard]] virtual bool IsKeyReleased(KeyboardKey key) const = 0;
			[[nodiscard]] virtual bool IsKeyUp(KeyboardKey key) = 0;

			[[nodiscard]] virtual bool IsMouseButtonPressed(MouseButton mouseButton) const = 0;
			[[nodiscard]] virtual bool IsMouseButtonDown(MouseButton mouseButton) const = 0;
			[[nodiscard]] virtual bool IsMouseButtonReleased(MouseButton mouseButton) const = 0;
			[[nodiscard]] virtual bool IsMouseButtonUp(MouseButton mouseButton) const = 0;

			[[nodiscard]] virtual bool IsGamepadButtonPressed(GamepadButton gamepadButton) const = 0;
			[[nodiscard]] virtual bool IsGamepadButtonDown(GamepadButton gamepadButton) const = 0;
			[[nodiscard]] virtual bool IsGamepadButtonReleased(GamepadButton gamepadButton) const = 0;
			[[nodiscard]] virtual bool IsGamepadButtonUp(GamepadButton gamepadButton) const = 0;

			[[nodiscard]] virtual InputState GetKeyState(KeyboardKey key) const = 0;
			[[nodiscard]] virtual InputState GetMouseButtonState(MouseButton mouseButton) const = 0;
			[[nodiscard]] virtual InputState GetGamepadButtonState(GamepadButton gamepadButton) const = 0;

			[[nodiscard]] virtual int GetMouseX() = 0;
			[[nodiscard]] virtual int GetMouseY() = 0;
			[[nodiscard]] virtual Vector2i GetMousePosition() = 0;

			[[nodiscard]] virtual float GetMouseDeltaX() = 0;
			[[nodiscard]] virtual float GetMouseDeltaY() = 0;
			[[nodiscard]] virtual Vector2f GetMouseDelta() = 0;

			[[nodiscard]] float GetSensitivity() const;

			[[nodiscard]] bool AreKeyModifiersPressed(KeyboardKeyWithModifier keyWithModifier) const;
			[[nodiscard]] bool AreMouseModifiersPressed(MouseButtonWithModifier mouseButtonWithModifier) const;

			[[nodiscard]] InputContext* AddContext(const std::string& name);
			void AddContext(const std::string& name, InputContext* context);
			void RemoveContext(const std::string& name);
			[[nodiscard]] InputContext* GetContext(const std::string& name);

		protected:

			/*
			 * Poll platform input
			 */
			virtual void PollInput() = 0;

			void UpdateAction(InputAction& action, Signal<InputEvent>* signal);

			float mMouseSensitivity;
			int mActiveGamepad = 0;

		private:

			void InitSettings();

			std::unordered_map<std::string, InputAction> mActions;
			std::unordered_map<std::string, Signal<InputEvent>*> mActionSignals;
			std::unordered_map<std::string, InputContext*> mContexts;
			std::unordered_map<std::string, bool> mManageContextLifetime;
			std::vector<std::string> mContextNamesInOrder;
		};
	}
}

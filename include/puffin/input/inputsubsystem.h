#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

//#include "GLFW/glfw3.h"

#include "puffin/core/subsystem.h"
#include "puffin/input/inputtypes.h"
#include "puffin/input/inputevent.h"
#include "puffin/types/vector2.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace input
	{
		class InputSubsystem : public core::Subsystem
		{
		public:

			explicit InputSubsystem(const std::shared_ptr<core::Engine>& engine);
			~InputSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			/*
			 * Called once a frame on input subsystem
			 */
			void ProcessInput();

			void AddAction(std::string name, int key);
			void AddAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction GetAction(std::string name) const;

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

			[[nodiscard]] virtual double GetMouseDeltaX() = 0;
			[[nodiscard]] virtual double GetMouseDeltaY() = 0;
			[[nodiscard]] virtual Vector2f GetMouseDelta() = 0;

			[[nodiscard]] double GetMouseXOffset() const;
			[[nodiscard]] double GetMouseYOffset() const;
			[[nodiscard]] double GetSensitivity() const;
			[[nodiscard]] bool GetCursorLocked() const;

			[[nodiscard]] bool AreKeyModifiersPressed(KeyboardKeyWithModifier keyWithModifier) const;
			[[nodiscard]] bool AreMouseModifiersPressed(MouseButtonWithModifier mouseButtonWithModifier) const;

		protected:

			/*
			 * Poll platform input
			 */
			virtual void PollInput() = 0;

			int mActiveGamepad = 0;

		private:

			double mXPos, mYPos, mLastXPos, mLastYPos;
			bool mCursorLocked;
			double mSensitivity;
			bool mFirstMouse;

			std::unordered_map<std::string, InputAction> mActions;
		};

		void AddEditorContext();
	}
}

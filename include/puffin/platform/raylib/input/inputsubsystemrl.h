#pragma once

#include "puffin/input/inputsubsystem.h"

namespace puffin::input
{
	class InputSubsystemRL : public InputSubsystem
	{
	public:

		explicit InputSubsystemRL(const std::shared_ptr<core::Engine>& engine);
		~InputSubsystemRL() override = default;

		bool IsKeyPressed(KeyboardKey key) const override;
		bool IsKeyDown(KeyboardKey key) const override;
		bool IsKeyReleased(KeyboardKey key) const override;
		bool IsKeyUp(KeyboardKey key) override;

		bool IsMouseButtonPressed(MouseButton mouseButton) const override;
		bool IsMouseButtonDown(MouseButton mouseButton) const override;
		bool IsMouseButtonReleased(MouseButton mouseButton) const override;
		bool IsMouseButtonUp(MouseButton mouseButton) const override;

		bool IsGamepadButtonPressed(GamepadButton gamepadButton) const override;
		bool IsGamepadButtonDown(GamepadButton gamepadButton) const override;
		bool IsGamepadButtonReleased(GamepadButton gamepadButton) const override;
		bool IsGamepadButtonUp(GamepadButton gamepadButton) const override;

		InputState GetKeyState(KeyboardKey key) const override;
		InputState GetMouseButtonState(MouseButton mouseButton) const override;
		InputState GetGamepadButtonState(GamepadButton gamepadButton) const override;

		int GetMouseX() override;
		int GetMouseY() override;
		Vector2i GetMousePosition() override;

		double GetMouseDeltaX() override;
		double GetMouseDeltaY() override;
		Vector2f GetMouseDelta() override;

	protected:

		void PollInput() override;
		

	private:



	};
}
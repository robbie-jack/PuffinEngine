#include "raylib/input/input_subsystem_rl.h"

#include "raylib-cpp.hpp"

namespace puffin::input
{
	InputSubsystemRL::InputSubsystemRL(const std::shared_ptr<core::Engine>& engine)
		: InputSubsystem(engine)
	{
	}

	bool InputSubsystemRL::IsKeyPressed(KeyboardKey key) const
	{
		return ::IsKeyPressed(static_cast<int>(key));
	}

	bool InputSubsystemRL::IsKeyDown(KeyboardKey key) const
	{
		return ::IsKeyDown(static_cast<int>(key));
	}

	bool InputSubsystemRL::IsKeyReleased(KeyboardKey key) const
	{
		return ::IsKeyReleased(static_cast<int>(key));
	}

	bool InputSubsystemRL::IsKeyUp(KeyboardKey key)
	{
		return ::IsKeyUp(static_cast<int>(key));
	}

	bool InputSubsystemRL::IsMouseButtonPressed(MouseButton mouseButton) const
	{
		return ::IsMouseButtonPressed(static_cast<int>(mouseButton));
	}

	bool InputSubsystemRL::IsMouseButtonDown(MouseButton mouseButton) const
	{
		return ::IsMouseButtonDown(static_cast<int>(mouseButton));
	}

	bool InputSubsystemRL::IsMouseButtonReleased(MouseButton mouseButton) const
	{
		return ::IsMouseButtonReleased(static_cast<int>(mouseButton));
	}

	bool InputSubsystemRL::IsMouseButtonUp(MouseButton mouseButton) const
	{
		return ::IsMouseButtonUp(static_cast<int>(mouseButton));
	}

	bool InputSubsystemRL::IsGamepadButtonPressed(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonPressed(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool InputSubsystemRL::IsGamepadButtonDown(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonDown(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool InputSubsystemRL::IsGamepadButtonReleased(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonReleased(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool InputSubsystemRL::IsGamepadButtonUp(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonUp(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	InputState InputSubsystemRL::GetKeyState(KeyboardKey key) const
	{
		if (::IsKeyPressed(static_cast<int>(key)))
			return InputState::Pressed;

		if (::IsKeyDown(static_cast<int>(key)))
			return InputState::Down;

		if (::IsKeyReleased(static_cast<int>(key)))
			return InputState::Released;

		if (::IsKeyUp(static_cast<int>(key)))
			return InputState::Up;

		return InputState::Up;
	}

	InputState InputSubsystemRL::GetMouseButtonState(MouseButton mouseButton) const
	{
		if (::IsMouseButtonPressed(static_cast<int>(mouseButton)))
			return InputState::Pressed;

		if (::IsMouseButtonDown(static_cast<int>(mouseButton)))
			return InputState::Down;

		if (::IsMouseButtonReleased(static_cast<int>(mouseButton)))
			return InputState::Released;

		if (::IsMouseButtonUp(static_cast<int>(mouseButton)))
			return InputState::Up;

		return InputState::Up;
	}

	InputState InputSubsystemRL::GetGamepadButtonState(GamepadButton gamepadButton) const
	{
		if (gamepadButton == GamepadButton::PaddleOne
			|| gamepadButton == GamepadButton::PaddleTwo
			|| gamepadButton == GamepadButton::PaddleThree
			|| gamepadButton == GamepadButton::PaddleFour)
			return InputState::Up;

		if (::IsGamepadButtonPressed(mActiveGamepad, static_cast<int>(gamepadButton)))
			return InputState::Pressed;

		if (::IsGamepadButtonDown(mActiveGamepad, static_cast<int>(gamepadButton)))
			return InputState::Down;

		if (::IsGamepadButtonReleased(mActiveGamepad, static_cast<int>(gamepadButton)))
			return InputState::Released;

		if (::IsGamepadButtonUp(mActiveGamepad, static_cast<int>(gamepadButton)))
			return InputState::Up;

		return InputState::Up;
	}

	int InputSubsystemRL::GetMouseX()
	{
		return ::GetMouseX();
	}

	int InputSubsystemRL::GetMouseY()
	{
		return ::GetMouseY();
	}

	Vector2i InputSubsystemRL::GetMousePosition()
	{
		auto mousePos = ::GetMousePosition();

		return { static_cast<int>(mousePos.x), static_cast<int>(mousePos.y) };
	}

	float InputSubsystemRL::GetMouseDeltaX()
	{
		return ::GetMouseDelta().x * mMouseSensitivity;
	}

	float InputSubsystemRL::GetMouseDeltaY()
	{
		return ::GetMouseDelta().y * mMouseSensitivity;
	}

	Vector2f InputSubsystemRL::GetMouseDelta()
	{
		auto mouseDelta = ::GetMouseDelta();

		return { mouseDelta.x * mMouseSensitivity,mouseDelta.y * mMouseSensitivity };
	}

	void InputSubsystemRL::PollInput()
	{
		PollInputEvents();
	}
}

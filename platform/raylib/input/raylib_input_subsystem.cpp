#include "raylib/input/raylib_input_subsystem.h"

#include "raylib-cpp.hpp"

namespace puffin::input
{
	RaylibInputSubsystem::RaylibInputSubsystem(const std::shared_ptr<core::Engine>& engine)
		: InputSubsystem(engine)
	{
	}

	std::string_view RaylibInputSubsystem::GetName() const
	{
		return reflection::GetTypeString<RaylibInputSubsystem>();
	}

	bool RaylibInputSubsystem::IsKeyPressed(KeyboardKey key) const
	{
		return ::IsKeyPressed(static_cast<int>(key));
	}

	bool RaylibInputSubsystem::IsKeyDown(KeyboardKey key) const
	{
		return ::IsKeyDown(static_cast<int>(key));
	}

	bool RaylibInputSubsystem::IsKeyReleased(KeyboardKey key) const
	{
		return ::IsKeyReleased(static_cast<int>(key));
	}

	bool RaylibInputSubsystem::IsKeyUp(KeyboardKey key)
	{
		return ::IsKeyUp(static_cast<int>(key));
	}

	bool RaylibInputSubsystem::IsMouseButtonPressed(MouseButton mouseButton) const
	{
		return ::IsMouseButtonPressed(static_cast<int>(mouseButton));
	}

	bool RaylibInputSubsystem::IsMouseButtonDown(MouseButton mouseButton) const
	{
		return ::IsMouseButtonDown(static_cast<int>(mouseButton));
	}

	bool RaylibInputSubsystem::IsMouseButtonReleased(MouseButton mouseButton) const
	{
		return ::IsMouseButtonReleased(static_cast<int>(mouseButton));
	}

	bool RaylibInputSubsystem::IsMouseButtonUp(MouseButton mouseButton) const
	{
		return ::IsMouseButtonUp(static_cast<int>(mouseButton));
	}

	bool RaylibInputSubsystem::IsGamepadButtonPressed(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonPressed(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool RaylibInputSubsystem::IsGamepadButtonDown(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonDown(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool RaylibInputSubsystem::IsGamepadButtonReleased(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonReleased(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	bool RaylibInputSubsystem::IsGamepadButtonUp(GamepadButton gamepadButton) const
	{
		return ::IsGamepadButtonUp(mActiveGamepad, static_cast<int>(gamepadButton));
	}

	InputState RaylibInputSubsystem::GetKeyState(KeyboardKey key) const
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

	InputState RaylibInputSubsystem::GetMouseButtonState(MouseButton mouseButton) const
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

	InputState RaylibInputSubsystem::GetGamepadButtonState(GamepadButton gamepadButton) const
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

	int RaylibInputSubsystem::GetMouseX()
	{
		return ::GetMouseX();
	}

	int RaylibInputSubsystem::GetMouseY()
	{
		return ::GetMouseY();
	}

	Vector2i RaylibInputSubsystem::GetMousePosition()
	{
		auto mousePos = ::GetMousePosition();

		return { static_cast<int>(mousePos.x), static_cast<int>(mousePos.y) };
	}

	float RaylibInputSubsystem::GetMouseDeltaX()
	{
		return ::GetMouseDelta().x * mMouseSensitivity;
	}

	float RaylibInputSubsystem::GetMouseDeltaY()
	{
		return ::GetMouseDelta().y * mMouseSensitivity;
	}

	Vector2f RaylibInputSubsystem::GetMouseDelta()
	{
		auto mouseDelta = ::GetMouseDelta();

		return { mouseDelta.x * mMouseSensitivity,mouseDelta.y * mMouseSensitivity };
	}

	void RaylibInputSubsystem::PollInput()
	{
		PollInputEvents();
	}
}

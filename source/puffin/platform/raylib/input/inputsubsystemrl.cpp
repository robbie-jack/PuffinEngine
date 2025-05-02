#include "puffin/platform/raylib/input/inputsubsystemrl.h"

#include "raylib-cpp.hpp"

namespace puffin::input
{
	InputSubsystemRL::InputSubsystemRL(const std::shared_ptr<core::Engine>& engine)
		: InputSubsystem(engine)
	{
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

	double InputSubsystemRL::GetMouseDeltaX()
	{
		return ::GetMouseDelta().x;
	}

	double InputSubsystemRL::GetMouseDeltaY()
	{
		return ::GetMouseDelta().y;
	}

	Vector2f InputSubsystemRL::GetMouseDelta()
	{
		auto mouseDelta = ::GetMouseDelta();

		return { mouseDelta.x, mouseDelta.y };
	}

	void InputSubsystemRL::PollInput()
	{
		PollInputEvents();
	}

	KeyState InputSubsystemRL::GetKeyState(KeyboardKey)
	{
		return KeyState::Up;
	}
}

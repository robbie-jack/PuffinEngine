#pragma once

#include <string>

namespace puffin::input
{
	enum class KeyState
	{
		JustPressed = 0,
		Pressed = 1,
		JustReleased = 2,
		Released = 3
	};

	struct InputEvent
	{
		InputEvent() : actionState(KeyState::Released) {}
		InputEvent(std::string name, KeyState state) : actionName(std::move(name)), actionState(state) {}

		std::string actionName;
		KeyState actionState;
	};
}

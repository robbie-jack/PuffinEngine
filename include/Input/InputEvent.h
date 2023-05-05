#pragma once

#include <string>

namespace puffin::input
{
	enum class KeyState
	{
		Pressed = 0,
		Held = 1,
		Released = 2,
		Up = 3
	};

	struct InputEvent
	{
		InputEvent() : actionState(KeyState::Up) {}
		InputEvent(std::string name, KeyState state) : actionName(std::move(name)), actionState(state) {}

		std::string actionName;
		KeyState actionState;
	};
}

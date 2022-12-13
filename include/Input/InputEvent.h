#pragma once

#include <string>

namespace Puffin::Input
{
	enum class KeyState
	{
		PRESSED = 0,
		HELD = 1,
		RELEASED = 2,
		UP = 3
	};

	struct InputEvent
	{
		InputEvent() : actionName{ "" }, actionState{ KeyState::UP } {};
		InputEvent(std::string name, KeyState state) : actionName{ name }, actionState{ state } {};

		std::string actionName;
		KeyState actionState;
	};
}

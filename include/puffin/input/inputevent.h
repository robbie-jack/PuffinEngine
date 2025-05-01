#pragma once

#include "inputtypes.h"

#include <string>

namespace puffin::input
{
	struct InputEvent
	{
		InputEvent() : actionState(KeyState::Up) {}
		InputEvent(std::string name, KeyState state) : actionName(std::move(name)), actionState(state) {}

		std::string actionName;
		KeyState actionState;
	};
}

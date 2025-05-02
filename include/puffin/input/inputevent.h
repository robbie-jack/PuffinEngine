#pragma once

#include "inputtypes.h"

#include <string>

namespace puffin::input
{
	struct InputEvent
	{
		InputEvent() : actionState(InputState::Up) {}
		InputEvent(std::string name, InputState state) : actionName(std::move(name)), actionState(state) {}

		std::string actionName;
		InputState actionState;
	};
}

#pragma once

#include <cstdint>

namespace puffin::input
{
	enum class KeyState
	{
		Pressed = 0,
		Down = 1,
		Released = 2,
		Up = 3
	};

	enum class KeyboardKey
	{
		
	};

	enum class MouseButton
	{
		
	};

	enum class GamepadButton
	{
		
	};

	enum class GamepadAxis
	{
		
	};

	struct InputAction
	{
		std::string name;
		int id = 0;
		std::vector<int> keys;
		KeyState state;
	};
}

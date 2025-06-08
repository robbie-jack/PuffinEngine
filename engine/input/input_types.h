#pragma once

#include <cstdint>

namespace puffin::input
{
	enum class InputState
	{
		Pressed = 0,
		Down = 1,
		Released = 2,
		Up = 3
	};

	enum class KeyboardKey
	{
		Null				= 0,

		// Alphanumeric
		Apostrophe			= 39, // Key: '
		Comma				= 44,			
		Minus				= 45,
		Period				= 46,
		Slash				= 47,
		Zero				= 48,
		One					= 49,
		Two					= 50,
		Three				= 51,
		Four				= 52,
		Five				= 53,
		Six					= 54,
		Seven				= 55,
		Eight				= 56,
		Nine				= 57,
		Semicolon			= 59,
		Equal				= 61,
		A					= 65,
		B					= 66,
		C					= 67,
		D					= 68,
		E					= 69,
		F					= 70,
		G					= 71,
		H					= 72,
		I					= 73,
		J					= 74,
		K					= 75,
		L					= 76,
		M					= 77,
		N					= 78,
		O					= 79,
		P					= 80,
		Q					= 81,
		R					= 82,
		S					= 83,
		T					= 84,
		U					= 85,
		V					= 86,
		W					= 87,
		X					= 88,
		Y					= 89,
		Z					= 90,
		LeftBracket			= 91,
		Backslash			= 92,
		RightBracket		= 93,
		Grave				= 96,

		// Function Keys
		Space				= 32,
		Escape				= 256,
		Enter				= 257,
		Tab					= 258,
		Backspace			= 259,
		Insert				= 260,
		Delete				= 261,
		Right				= 262,
		Left				= 263,
		Up					= 264,
		Down				= 265,
		PageUp				= 266,
		PageDown			= 267,
		Home				= 268,
		End					= 269,
		CapsLock			= 280,
		ScrollLock			= 281,
		NumLock				= 282,
		PrintScreen			= 283,
		Pause				= 284,
		F1					= 290,
		F2					= 291,
		F3					= 292,
		F4					= 293,
		F5					= 294,
		F6					= 295,
		F7					= 296,
		F8					= 297,
		F9					= 298,
		F10					= 299,
		F11					= 300,
		F12					= 301,
		LeftShift			= 340,
		LeftControl			= 341,
		LeftAlt				= 342,
		LeftSuper			= 343,
		RightShift			= 344,
		RightControl		= 345,
		RightAlt			= 346,
		RightSuper			= 347,

		// Keypad
		KpZero				= 320,
		KpOne				= 321,
		KpTwo				= 322,
		KpThree				= 323,
		KpFour				= 324,
		KpFive				= 325,
		KpSix				= 326,
		KpSeven				= 327,
		KpEight				= 328,
		KpNine				= 329,
		KpDecimal			= 330,
		KpMultiply			= 331,
		KpDivide			= 332,
		KpSubtract			= 333,
		KpAdd				= 334,
		KpEnter				= 335,
		KpEqual				= 336
	};

	enum class MouseButton
	{
		One					= 0,
		Two					= 1,
		Three				= 2,
		Four				= 3,
		Five				= 4,

		// Mouse Aliases
		Left				= One,
		Right				= Two,
		Middle				= Three,
		Side				= Four,
		Extra				= Five
	};

	enum class GamepadButton
	{
		DPadUp				= 1,
		DPadRight,
		DPadDown,
		DPadLeft,
		FaceUp,						// Playstation: Triangle, Xbox: Y, Nintendo: X
		FaceRight,					// Playstation: Circle, Xbox: B, Nintendo: A
		FaceDown,					// Playstation: Cross, Xbox: A, Nintendo: B
		FaceLeft,					// Playstation: Square, Xbox: X, Nintendo: Y
		LeftTriggerPartial,
		LeftTriggerFull,
		RightTriggerPartial,
		RightTriggerFull,
		MiddleLeft,
		MiddleCentre,
		MiddleRight,
		LeftThumb,
		RightThumb,
		PaddleOne,
		PaddleTwo,
		PaddleThree,
		PaddleFour
	};

	enum class GamepadAxis
	{
		LeftX				= 0,
		LeftY				= 1,
		RightX				= 2,
		RightY				= 3,
		LeftTrigger			= 4,
		RightTrigger		= 5,
		GyroX				= 6,
		GyroY				= 7
	};

	/*
	 *	Struct defining a keyboard key input with optional control, alt and shift modifier
	 */
	struct KeyboardKeyWithModifier
	{
		KeyboardKeyWithModifier() = default;
		KeyboardKeyWithModifier(KeyboardKey key, bool ctrlPressed = false, bool altPressed = false, bool shiftPressed = false)
			: key(key), ctrlPressed(ctrlPressed), altPressed(altPressed), shiftPressed(shiftPressed)
		{}

		KeyboardKey key;
		bool ctrlPressed = false;
		bool altPressed = false;
		bool shiftPressed = false;
	};

	struct MouseButtonWithModifier
	{
		MouseButtonWithModifier() = default;
		MouseButtonWithModifier(MouseButton button, bool ctrlPressed = false, bool altPressed = false, bool shiftPressed = false)
			: button(button), ctrlPressed(ctrlPressed), altPressed(altPressed), shiftPressed(shiftPressed)
		{
		}

		MouseButton button;
		bool ctrlPressed = false;
		bool altPressed = false;
		bool shiftPressed = false;
	};

	struct InputAction
	{
		explicit InputAction(std::string name) : name(std::move(name)) {}

		std::string name;
		std::vector<KeyboardKeyWithModifier> keys;
		std::vector<MouseButtonWithModifier> mouseButtons;
		std::vector<GamepadButton> gamepadButtons;
		std::vector<GamepadAxis> gamepadAxis;
		InputState state = InputState::Up;
		InputState lastState = InputState::Up;
	};
}

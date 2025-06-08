#include "input/input_context.h"

#include <cassert>

namespace puffin::input
{
	InputContext::InputContext(std::string name) : mName(std::move(name))
	{
	}

	InputAction& InputContext::AddAction(const std::string& name)
	{
		assert(mActions.find(name) == mActions.end() && "InputContext::AddAction - Context with that name already exists");

		mActions.emplace(name, InputAction(name));

		return mActions.at(name);
	}

	void InputContext::AddAction(const std::string& name, const InputAction& action)
	{
		assert(mActions.find(name) == mActions.end() && "InputContext::AddAction - Context with that name already exists");

		mActions.emplace(name, action);
	}

	void InputContext::RemoveAction(const std::string& name)
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::RemoveAction - Context with that name doesn't exist");

		mActions.erase(name);
	}

	InputAction& InputContext::GetAction(const std::string& name)
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::GetAction - Context with that name doesn't exist");

		return mActions.at(name);
	}

	const InputAction& InputContext::GetAction(const std::string& name) const
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::GetAction - Context with that name doesn't exist");

		return mActions.at(name);
	}

	std::unordered_map<std::string, InputAction>& InputContext::GetActions()
	{
		return mActions;
	}

	bool InputContext::HasAction(const std::string& name)
	{
		return mActions.find(name) != mActions.end();
	}

	bool InputContext::IsActionPressed(const std::string& name) const
	{
		if (mActions.find(name) != mActions.end())
			return mActions.at(name).state == InputState::Pressed;

		return false;
	}

	bool InputContext::IsActionDown(const std::string& name) const
	{
		if (mActions.find(name) != mActions.end())
			return mActions.at(name).state == InputState::Down;

		return false;
	}

	bool InputContext::IsActionReleased(const std::string& name) const
	{
		if (mActions.find(name) != mActions.end())
			return mActions.at(name).state == InputState::Released;

		return false;
	}

	bool InputContext::IsActionUp(const std::string& name) const
	{
		if (mActions.find(name) != mActions.end())
			return mActions.at(name).state == InputState::Up;

		return false;
	}

	void InputContext::SetBlockInput(bool blockInput)
	{
		mBlockInput = blockInput;
	}

	bool InputContext::GetBlockInput() const
	{
		return mBlockInput;
	}
}

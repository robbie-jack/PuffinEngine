#include "input/input_context.h"

#include <cassert>

namespace puffin::input
{
	InputContext::InputContext(std::string name) : mName(std::move(name))
	{
	}

	InputContext::~InputContext()
	{
		mActions.clear();

		for (auto& [name, signal] : mActionSignals)
		{
			delete signal;
		}

		mActionSignals.clear();
	}

	InputAction& InputContext::AddAction(const std::string& name)
	{
		assert(mActions.find(name) == mActions.end() && "InputContext::AddAction - Action with that name already exists");

		mActions.emplace(name, InputAction(name));
		mActionSignals.emplace(name, new Signal<InputEvent>());

		return mActions.at(name);
	}

	void InputContext::AddAction(const std::string& name, const InputAction& action)
	{
		assert(mActions.find(name) == mActions.end() && "InputContext::AddAction - Action with that name already exists");

		mActions.emplace(name, action);
		mActionSignals.emplace(name, new Signal<InputEvent>());
	}

	void InputContext::RemoveAction(const std::string& name)
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::RemoveAction - Action with that name doesn't exist");

		mActions.erase(name);
		mActionSignals.erase(name);
	}

	InputAction& InputContext::GetAction(const std::string& name)
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::GetAction - Action with that name doesn't exist");

		return mActions.at(name);
	}

	const InputAction& InputContext::GetAction(const std::string& name) const
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::GetAction - Action with that name doesn't exist");

		return mActions.at(name);
	}

	Signal<InputEvent>* InputContext::GetActionSignal(const std::string& name)
	{
		assert(mActions.find(name) != mActions.end() && "InputContext::GetAction - Action signal with that name doesn't exist");

		return mActionSignals.at(name);
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

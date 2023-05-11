#pragma once

#include "Subsystem.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <functional>

namespace puffin::core
{
	class ISignalHandler
	{
	};

	template<typename SignalT>
	class SignalHandler : public ISignalHandler
	{
	public:

		SignalHandler(const std::function<void(const SignalT&)>& func) : m_func(func) {}
		~SignalHandler() = default;

		void signal(const SignalT& signal)
		{
			m_func(signal);
		}

	private:

		std::function<void(const SignalT&)> m_func;

	};

	typedef std::vector<std::shared_ptr<ISignalHandler>> SignalHandlerVector;

	class SignalSubsystem : public Subsystem
	{
	public:

		SignalSubsystem() = default;

		~SignalSubsystem() override = default;

		void setupCallbacks() override;

		void shutdown()
		{
			mSignalHandlers.clear();
		}

		template<typename SignalT>
		void signal(const SignalT& signal)
		{
			if (const std::type_index typeIndex = typeid(SignalT); mSignalHandlers.count(typeIndex) == 1)
			{
				// Check if any signals of this type have been connected
				for (int i = 0; i < mSignalHandlers[typeIndex].size(); i++)
				{
					getSignalHandler<SignalT>(i)->signal(signal);
				}
			}
		}

		template<typename SignalT>
		void connect(const std::function<void(const SignalT&)>& func)
		{
			const std::type_index typeIndex = typeid(SignalT);

			// Create signal vector if one does not yet exist
			if (mSignalHandlers.count(typeIndex) == 0)
			{
				mSignalHandlers.emplace(typeIndex, SignalHandlerVector());
			}

			// Create new signal handler
			mSignalHandlers[typeIndex].push_back(std::make_shared<SignalHandler<SignalT>>(func));
		}

	private:

		std::unordered_map<std::type_index, SignalHandlerVector> mSignalHandlers;

		template<typename SignalT>
		std::shared_ptr<SignalHandler<SignalT>> getSignalHandler(const size_t index)
		{
			const std::type_index typeIndex = typeid(SignalT);

			if (mSignalHandlers.count(typeIndex) == 0 || mSignalHandlers[typeIndex].empty())
			{
				return nullptr;
			}

			return std::static_pointer_cast<SignalHandler<SignalT>>(mSignalHandlers[typeIndex][index]);
		}
	};
}
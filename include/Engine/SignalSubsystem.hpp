#pragma once

#include "Engine/Subsystem.hpp"

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

		void Signal(const SignalT& signal)
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

		void SetupCallbacks() override;

		void Cleanup()
		{
			m_signalHandlers.clear();
		}

		template<typename SignalT>
		void Signal(const SignalT& signal)
		{
			const std::type_index typeIndex = typeid(SignalT);

			if (m_signalHandlers.count(typeIndex) == 1)
			{
				// Check if any signals of this type have been connected
				for (int i = 0; i < m_signalHandlers[typeIndex].size(); i++)
				{
					GetSignalHandler<SignalT>(i)->Signal(signal);
				}
			}
		}

		template<typename SignalT>
		void Connect(const std::function<void(const SignalT&)>& func)
		{
			const std::type_index typeIndex = typeid(SignalT);

			// Create signal vector if one does not yet exist
			if (m_signalHandlers.count(typeIndex) == 0)
			{
				m_signalHandlers.emplace(typeIndex, SignalHandlerVector());
			}

			// Create new signal handler
			m_signalHandlers[typeIndex].push_back(std::make_shared<SignalHandler<SignalT>>(func));
		}

	private:

		std::unordered_map<std::type_index, SignalHandlerVector> m_signalHandlers;

		template<typename SignalT>
		std::shared_ptr<SignalHandler<SignalT>> GetSignalHandler(size_t index)
		{
			const std::type_index typeIndex = typeid(SignalT);

			if (m_signalHandlers.count(typeIndex) == 0 || m_signalHandlers[typeIndex].empty())
			{
				return nullptr;
			}

			return std::static_pointer_cast<SignalHandler<SignalT>>(m_signalHandlers[typeIndex][index]);
		}
	};
}
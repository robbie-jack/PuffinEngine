#pragma once

#include "Engine/Subsystem.hpp"

#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <functional>

namespace Puffin::Core
{
	class ISignalHandler
	{
	public:

		//virtual ~ISignalHandler() = 0;

	};

	//template<typename SignalT>
	//class SignalHandlerBase : public ISignalHandler
	//{
	//public:

	//	~SignalHandlerBase() override = 0;

	//	virtual void Signal(const SignalT& signal) = 0;

	//};

	//template<typename InstT, typename SignalT>
	//class SignalHandler : public  SignalHandlerBase<SignalT>
	//{
	//public:
	//	
	//	typedef void (InstT::*MemberFunction)(const SignalT&);

	//	SignalHandler(std::shared_ptr<InstT> instance, MemberFunction memberFunction) : m_instance(instance), m_memberFunction(memberFunction) {}
	//	~SignalHandler() override = default;

	//	void Signal(const SignalT& signal) override
	//	{
	//		//(m_instance->*m_memberFunction)(signal);
	//	}

	//private:

	//	std::shared_ptr<InstT> m_instance = nullptr;

	//	MemberFunction m_memberFunction;

	//};

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

		SignalSubsystem()
		{
			m_shouldUpdate = false;
		}

		~SignalSubsystem() override = default;

		void Init() override {}
		void Update() override {}
		void Destroy() override
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
#pragma once

#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

#include "puffin/core/subsystem.h"
#include "puffin/types/packed_vector.h"

namespace puffin::core
{
	template<typename... Params>
	class Slot
	{
	public:

		Slot(const std::function<void(const Params&...)>& callback) : mCallback(callback) {}
		~Slot() = default;

		void execute(Params... args) const
		{
			mCallback(args...);
		}

	private:

		std::function<void(const Params&...)> mCallback;

	};

	class ISignal
	{
	public:

		virtual ~ISignal() = default;
		virtual const std::string& name() = 0;

	};

	template<typename... Params>
	class Signal final : public ISignal
	{
	public:

		Signal(const std::string& name) : mName(name) {}
		~Signal() override = default;

		size_t connect(const std::function<void(const Params&...)>& callback)
		{
			size_t slotID = mNextSlotID;
			mNextSlotID++;

			mSlots.emplace(slotID, Slot<Params...>(callback));

			return slotID;
		}

		void disconnect(const size_t& slotID)
		{
			mSlots.erase(slotID);
		}

		void emit(Params... params)
		{
			for (const Slot<Params...>& slot : mSlots)
			{
				slot.execute(params...);
			}
		}

		const std::string& name() override { return mName; }

	private:

		std::string mName;
		size_t mNextSlotID = 1;
		PackedVector<size_t, Slot<Params...>> mSlots;

	};

	typedef std::shared_ptr<ISignal> ISignalPtr;

	class SignalSubsystem : public Subsystem
	{
	public:

		explicit SignalSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine) {}

		~SignalSubsystem() override
		{
			m_engine = nullptr;
			m_signals.clear();
		}

		template<typename... Params>
		std::shared_ptr<Signal<Params...>> create_signal(const std::string& name)
		{
			m_signals.emplace(name, std::make_shared<Signal<Params...>>(name));

			return std::static_pointer_cast<Signal<Params...>>(m_signals.at(name));
		}

		template<typename... Params>
		void add_signal(std::shared_ptr<Signal<Params...>> signal)
		{
			m_signals.emplace(signal->name(), signal);
		}

		template<typename... Params>
		std::shared_ptr<Signal<Params...>> get_signal(const std::string& name)
		{
			if (m_signals.count(name) == 0)
			{
				return nullptr;
			}

			return std::static_pointer_cast<Signal<Params...>>(m_signals.at(name));
		}

		template<typename... Params>
		size_t connect(const std::string& name, const std::function<void(const Params&...)>& callback)
		{
			if (m_signals.count(name) == 0)
			{
				return 0;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(m_signals.at(name));
			return signal->connect(callback);
		}

		template<typename... Params>
		void disconnect(const std::string& name, const size_t& slotID)
		{
			if (m_signals.count(name) == 0)
			{
				return;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(m_signals.at(name));
			signal->disconnenct(slotID);
		}

		template<typename... Params>
		bool emit(const std::string& name, Params... params)
		{
			if (m_signals.count(name) == 0)
			{
				return false;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(m_signals.at(name));
			signal->emit(params...);

			return true;
		}

	private:

		std::unordered_map<std::string, ISignalPtr> m_signals;

	};
}

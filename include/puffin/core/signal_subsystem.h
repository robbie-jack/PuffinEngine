#pragma once

#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

#include "puffin/core/system.h"
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

	class SignalSubsystem : public System
	{
	public:

		SignalSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine) {}

		~SignalSubsystem() override
		{
			mEngine = nullptr;
			mSignals.clear();
		}

		template<typename... Params>
		std::shared_ptr<Signal<Params...>> createSignal(const std::string& name)
		{
			mSignals.emplace(name, std::make_shared<Signal<Params...>>(name));

			return std::static_pointer_cast<Signal<Params...>>(mSignals.at(name));
		}

		template<typename... Params>
		void addSignal(std::shared_ptr<Signal<Params...>> signal)
		{
			mSignals.emplace(signal->name(), signal);
		}

		template<typename... Params>
		std::shared_ptr<Signal<Params...>> getSignal(const std::string& name)
		{
			if (mSignals.count(name) == 0)
			{
				return nullptr;
			}

			return mSignals.at(name);
		}

		template<typename... Params>
		size_t connect(const std::string& name, const std::function<void(const Params&...)>& callback)
		{
			if (mSignals.count(name) == 0)
			{
				return 0;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(mSignals.at(name));
			return signal->connect(callback);
		}

		template<typename... Params>
		void disconnect(const std::string& name, const size_t& slotID)
		{
			if (mSignals.count(name) == 0)
			{
				return;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(mSignals.at(name));
			signal->disconnenct(slotID);
		}

		template<typename... Params>
		bool emit(const std::string& name, Params... params)
		{
			if (mSignals.count(name) == 0)
			{
				return false;
			}

			auto signal = std::static_pointer_cast<Signal<Params...>>(mSignals.at(name));
			signal->emit(params...);

			return true;
		}

	private:

		std::unordered_map<std::string, ISignalPtr> mSignals;

	};
}

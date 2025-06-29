#pragma once

#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

#include "core/subsystem.h"
#include "types/storage/mapped_vector.h"

namespace puffin::core
{
	template<typename... Params>
	class Slot
	{
	public:

		explicit Slot(const std::function<void(const Params&...)>& Callback) : mCallback(Callback) {}
		~Slot() = default;

		void Execute(Params... args) const
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
		virtual const std::string& GetName() = 0;

	};

	template<typename... Parameters>
	class Signal final : public ISignal
	{
	public:

		Signal(const std::string& name) : mName(name) {}
		~Signal() override = default;

		size_t Connect(const std::function<void(const Parameters&...)>& Callback)
		{
			size_t slotID = mNextSlotID;
			mNextSlotID++;

			mSlots.Emplace(slotID, Slot<Parameters...>(Callback));

			return slotID;
		}

		void Disconnect(const size_t& slotID)
		{
			mSlots.Erase(slotID);
		}

		void Emit(Parameters... params)
		{
			for (const Slot<Parameters...>& slot : mSlots)
			{
				slot.Execute(params...);
			}
		}

		const std::string& GetName() override { return mName; }

	private:

		std::string mName;
		size_t mNextSlotID = 1;
		MappedVector<size_t, Slot<Parameters...>> mSlots;

	};

	typedef std::shared_ptr<ISignal> ISignalPtr;

	class SignalSubsystem : public Subsystem
	{
	public:

		explicit SignalSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
		{
			mName = "SignalSubsystem";
		}

		~SignalSubsystem() override
		{
			m_engine = nullptr;
			mSignals.clear();
		}

		template<typename... Parameters>
		std::shared_ptr<Signal<Parameters...>> CreateSignal(const std::string& name)
		{
			mSignals.emplace(name, std::make_shared<Signal<Parameters...>>(name));

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(name));
		}

		template<typename... Parameters>
		void AddSignal(std::shared_ptr<Signal<Parameters...>> signal)
		{
			mSignals.emplace(signal->GetName(), signal);
		}

		template<typename... Parameters>
		std::shared_ptr<Signal<Parameters...>> GetSignal(const std::string& name)
		{
			if (mSignals.count(name) == 0)
			{
				return nullptr;
			}

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(name));
		}
		
		template<typename... Parameters>
		std::shared_ptr<Signal<Parameters...>> GetOrCreateSignal(const std::string& name)
		{
			auto signal = GetSignal<Parameters...>(name);

			if (!signal)
			{
				signal = CreateSignal<Parameters...>(name);
			}
			
			return signal;
		}

		template<typename... Parameters>
		size_t Connect(const std::string& name, const std::function<void(const Parameters&...)>& callback)
		{
			if (mSignals.count(name) == 0)
			{
				return 0;
			}

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(name))->Connect(callback);
		}

		template<typename... Parameters>
		void Disconnect(const std::string& name, const size_t& slotID)
		{
			if (mSignals.count(name) == 0)
			{
				return;
			}

			std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(name))->Disconnect(slotID);
		}

		template<typename... Parameters>
		bool Emit(const std::string& name, Parameters... params)
		{
			if (mSignals.count(name) == 0)
			{
				return false;
			}

			std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(name))->Emit(params...);

			return true;
		}

	private:

		std::unordered_map<std::string, ISignalPtr> mSignals;

	};
}

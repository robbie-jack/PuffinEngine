#pragma once

#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

#include "puffin/core/subsystem.h"
#include "puffin/types/packedvector.h"

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

		Signal(const std::string& Name) : mName(Name) {}
		~Signal() override = default;

		size_t Connect(const std::function<void(const Parameters&...)>& Callback)
		{
			size_t SlotID = mNextSlotID;
			mNextSlotID++;

			mSlots.emplace(SlotID, Slot<Parameters...>(Callback));

			return SlotID;
		}

		void Disconnect(const size_t& SlotID)
		{
			mSlots.erase(SlotID);
		}

		void Emit(Parameters... Params)
		{
			for (const Slot<Parameters...>& Slot : mSlots)
			{
				Slot.Execute(Params...);
			}
		}

		const std::string& GetName() override { return mName; }

	private:

		std::string mName;
		size_t mNextSlotID = 1;
		PackedVector<size_t, Slot<Parameters...>> mSlots;

	};

	typedef std::shared_ptr<ISignal> ISignalPtr;

	class SignalSubsystem : public Subsystem
	{
	public:

		explicit SignalSubsystem(const std::shared_ptr<core::Engine>& Engine) : Subsystem(Engine)
		{
			mName = "SignalSubsystem";
		}

		~SignalSubsystem() override
		{
			mEngine = nullptr;
			mSignals.clear();
		}

		template<typename... Parameters>
		std::shared_ptr<Signal<Parameters...>> CreateSignal(const std::string& Name)
		{
			mSignals.emplace(Name, std::make_shared<Signal<Parameters...>>(Name));

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(Name));
		}

		template<typename... Parameters>
		void AddSignal(std::shared_ptr<Signal<Parameters...>> Signal)
		{
			mSignals.emplace(Signal->GetName(), Signal);
		}

		template<typename... Parameters>
		std::shared_ptr<Signal<Parameters...>> GetSignal(const std::string& Name)
		{
			if (mSignals.count(Name) == 0)
			{
				return nullptr;
			}

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(Name));
		}

		template<typename... Parameters>
		size_t Connect(const std::string& Name, const std::function<void(const Parameters&...)>& Callback)
		{
			if (mSignals.count(Name) == 0)
			{
				return 0;
			}

			return std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(Name))->Connect(Callback);
		}

		template<typename... Parameters>
		void Disconnect(const std::string& Name, const size_t& SlotID)
		{
			if (mSignals.count(Name) == 0)
			{
				return;
			}

			std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(Name))->Disconnect(SlotID);
		}

		template<typename... Parameters>
		bool Emit(const std::string& Name, Parameters... Params)
		{
			if (mSignals.count(Name) == 0)
			{
				return false;
			}

			std::static_pointer_cast<Signal<Parameters...>>(mSignals.at(Name))->Emit(Params...);

			return true;
		}

	private:

		std::unordered_map<std::string, ISignalPtr> mSignals;

	};
}

#pragma once

#include <functional>

#include "types/storage/mapped_vector.h"

template<typename... Params>
class Connection
{
public:

	Connection() = default;
	Connection(const std::function<void(const Params&...)>& callback)
		: mCallback(callback) {}
	~Connection() = default;

	void Execute(const Params&... args) const
	{
		mCallback(args...);
	}

private:

	std::function<void(const Params&...)> mCallback;

};

template<typename... Parameters>
class Signal
{
public:

	Signal()
	{
		mConnections.Resize(100);
	}

	~Signal()
	{
		mConnections.Clear();
	}

	size_t Connect(const std::function<void(const Parameters&...)>& callback)
	{
		const size_t connectionId = mNextConnectionId;
		mNextConnectionId++;

		mConnections.Emplace(connectionId, Connection<Parameters...>(callback));

		return connectionId;
	}

	void Disconnect(const size_t& connectionId)
	{
		if (!mConnections.Contains(connectionId))
			return;

		mConnections.Erase(connectionId);
	}

	void Emit(const Parameters&... parameters)
	{
		for (auto& connection : mConnections)
		{
			connection.Execute(parameters...);
		}
	}

protected:



private:

	size_t mNextConnectionId = 1;

	puffin::MappedVector<size_t, Connection<Parameters...>> mConnections;

};
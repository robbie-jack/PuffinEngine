#pragma once

#include "Types/UUID.h"

namespace puffin::scene
{
	class INodeFactory
	{
	public:

		INodeFactory() {}

		virtual ~INodeFactory() = default;

		virtual void create(const PuffinID& id = gInvalidID) = 0;

	protected:

	};

	template<typename T>
	class NodeFactory final : public INodeFactory
	{
	public:

		NodeFactory() {}

		T create(const PuffinID& id = gInvalidID) override
		{
			return T(id);
		}

	private:



	};

	class SceneGraph
	{
	public:



	private:



	};
}

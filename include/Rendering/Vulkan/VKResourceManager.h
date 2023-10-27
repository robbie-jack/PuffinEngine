#pragma once

#include "Types/UUID.h"

#include <string>
#include <unordered_set>

namespace puffin::rendering
{
	class VKRenderResource
	{
	public:

		enum class Type
		{
			Buffer,
			Image
		};

		void addWriteNode(PuffinID id)
		{
			mWriteNodes.insert(id);
		}

		void addReadNode(PuffinID id)
		{
			mReadNodes.insert(id);
		}

	private:

		Type mType = Type::Buffer;

		std::unordered_set<PuffinID> mWriteNodes;
		std::unordered_set<PuffinID> mReadNodes;

	};

	class VKResourceManager
	{
	public:

		VKResourceManager() = default;
		~VKResourceManager() = default;

		// PUFFIN_TODO - Implement Methods

		PuffinID createImage(const std::string& name)
		{
			return PuffinID();
		}

	private:



	};
}

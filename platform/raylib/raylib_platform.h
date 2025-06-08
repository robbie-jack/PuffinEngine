#pragma once

#include "platform.h"

namespace puffin::core
{
	class PlatformRL : public puffin::core::Platform
	{
	public:

		PlatformRL(std::shared_ptr<Engine> engine);
		~PlatformRL() override = default;

		void RegisterTypes() override;

	};
}
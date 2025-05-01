#pragma once

#include "puffin/platform/platform.h"

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
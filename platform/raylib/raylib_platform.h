#pragma once

#include "platform.h"

namespace puffin::core
{
	class RaylibPlatform : public puffin::core::Platform
	{
	public:

		RaylibPlatform(std::shared_ptr<Engine> engine);
		~RaylibPlatform() override = default;

		void RegisterTypes() override;

	};
}
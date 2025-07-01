#pragma once

#include "platform.h"

namespace puffin::core
{
	class RaylibPlatform : public puffin::core::Platform
	{
	public:

		RaylibPlatform(std::shared_ptr<Engine> engine);
		~RaylibPlatform() override = default;

		void PreInitialize() override;
		void Initialize() override;
		void PostInitialize() override;
		void Deinitialize() override;
	};
}
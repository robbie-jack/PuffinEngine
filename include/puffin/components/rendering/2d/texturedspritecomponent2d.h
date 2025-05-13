#pragma once

#include <filesystem>

#include "puffin/components/rendering/2d/spritecomponent2d.h"

namespace puffin
{
	namespace rendering
	{
		struct TexturedSpriteComponent2D : public SpriteComponent2D
		{
			std::filesystem::path path;
		};
	}
}
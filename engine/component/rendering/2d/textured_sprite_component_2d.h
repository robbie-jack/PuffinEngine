#pragma once

#include <filesystem>

#include "component/rendering/2d/sprite_component_2d.h"

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
#pragma once

#include "component/rendering/2d/sprite_component_2d.h"
#include "types/vector2.h"

namespace puffin
{
	namespace rendering
	{
		struct RectangleSpriteComponent2D : public SpriteComponent2D
		{
			Vector2i size;
		};
	}
}

#pragma once

#include "puffin/components/rendering/2d/spritecomponent2d.h"
#include "puffin/types/vector2.h"

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

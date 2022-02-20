#pragma once

#ifndef MATHS_HELPERS_H
#define MATHS_HELPERS_H

#include <Types/Vector.h>

#include <algorithm>

namespace Puffin
{
	namespace Maths
	{
		inline Float Clamp(const Float& Value, const Float& Min, const Float& Max)
		{
			return std::max(Min, std::min(Max, Value));
		}

		inline Vector2 Clamp(const Vector2& Vector, const Vector2& Min, const Vector2& Max)
		{
			return Vector2(Clamp(Vector.x, Min.x, Max.x), Clamp(Vector.y, Min.y, Max.y));
		}
	}
}

#endif //MATHS_HELPERS_H
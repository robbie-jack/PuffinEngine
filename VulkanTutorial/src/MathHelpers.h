#pragma once

#ifndef MATHS_HELPERS_H
#define MATHS_HELPERS_H

#include <Types/Vector.h>

#include <algorithm>

namespace Puffin
{
	namespace Maths
	{
		template<typename T>
		inline T Clamp(const T& Value, const T& Min, const T& Max)
		{
			return std::max(Min, std::min(Max, Value));
		}

		inline Vector2f Clamp(const Vector2f& Vector, const Vector2f& Min, const Vector2f& Max)
		{
			return Vector2f(Clamp(Vector.x, Min.x, Max.x), Clamp(Vector.y, Min.y, Max.y));
		}
	}
}

#endif //MATHS_HELPERS_H
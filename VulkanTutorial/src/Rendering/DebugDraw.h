#pragma once

#include <Types/Vector.h>

#include <vector>

namespace Puffin
{
	namespace Debug
	{
		struct Line
		{
			Line() {};
			Line(Vector3 InStart, Vector3 InEnd, Vector3 InColor)
				: start{ InStart }, end{ InEnd }, color{ InColor } {};

			Vector3 start;
			Vector3 end;
			Vector3 color;
		};

		struct Box
		{
			Box() {};
			Box(Vector3 InOrigin, Vector3 InHalfSize, Vector3 InColor)
				: origin{ InOrigin }, halfSize{ InHalfSize }, color{ InColor } {};

			Vector3 origin;
			Vector3 halfSize;
			Vector3 color;
		};

		struct Circle
		{
			Vector3 origin;
			float radius;
			int segements;
		};

		struct Grid
		{
			Vector3 origin;
			Vector3 halfSize;
			int rows, columns;
		};
	}
}
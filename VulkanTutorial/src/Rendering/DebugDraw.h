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
			Line(Vector3f InStart, Vector3f InEnd, Vector3f InColor)
				: start{ InStart }, end{ InEnd }, color{ InColor } {};

			Vector3f start;
			Vector3f end;
			Vector3f color;
		};

		struct Box
		{
			Box() {};
			Box(Vector2f InOrigin, Vector2f InHalfSize, Vector3f InColor)
				: origin{ InOrigin }, halfSize{ InHalfSize }, color{ InColor } {};

			Vector2f origin;
			Vector2f halfSize;
			float angle;
			Vector3f color;
		};

		struct Cube
		{
			Cube() {};
			Cube(Vector3f InOrigin, Vector3f InHalfSize, Vector3f InColor)
				: origin{ InOrigin }, halfSize{ InHalfSize }, color{ InColor } {};

			Vector3f origin;
			Vector3f halfSize;
			Vector3f color;
		};

		struct Circle
		{
			Vector3f origin;
			float radius;
			int segements;
		};

		struct Grid
		{
			Vector3f origin;
			Vector3f halfSize;
			int rows, columns;
		};
	}
}
#pragma once

#include <Types/Vector.h>

#include <vector>

namespace Puffin
{
	namespace Debug
	{
		struct DebugLine
		{
			Vector3 start;
			Vector3 end;
			Vector3 color;
		};

		struct DebugBox
		{
			Vector3 origin;
			Vector3 halfSize;
			Vector3 color;
		};

		struct DebugCircle
		{
			Vector3 origin;
			float radius;
			int segements;
		};

		struct DebugGrid
		{
			Vector3 origin;
			Vector3 halfSize;
			int rows, columns;
		};

		extern std::vector<DebugLine> debugLines;
		extern std::vector<DebugBox> debugBoxes;

		// Draw Functions - Used to store draw data for renderer
		inline void DrawLine(Vector3 start, Vector3 end, Vector3 color)
		{
			DebugLine debugLine;
			debugLine.start = start;
			debugLine.end = end;
			debugLine.color = color;

			debugLines.push_back(debugLine);
		}

		inline void DrawBox(Vector3 origin, Vector3 halfSize, Vector3 color)
		{
			DebugBox debugBox;
			debugBox.origin = origin;
			debugBox.halfSize = halfSize;
			debugBox.color = color;

			debugBoxes.push_back(debugBox);
		}

		inline void DrawCircle(Vector3 origin, float radius, int segments)
		{

		}

		inline void DrawGrid(Vector3 origin, Vector2 halfSize, int rows, int columns)
		{

		}
	}
}
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

		// Retrieve Functions - used by renderer to retrieve draw data
		inline std::vector<DebugLine> RetrieveDrawLines()
		{
			std::vector<DebugLine> lines = debugLines;
			debugLines.clear();
			return lines;
		}

		inline std::vector<DebugBox> RetrieveDrawBoxes()
		{
			std::vector<DebugBox> boxes = debugBoxes;
			debugBoxes.clear();
			return boxes;
		}
	}
}
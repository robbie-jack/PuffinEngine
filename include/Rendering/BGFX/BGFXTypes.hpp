#pragma once

#include "BGFXVertex.hpp"

namespace Puffin::Rendering::BGFX
{
	struct MeshStagingData
	{
		VertexPNTV32Buffer vertices;
		VertexBufferIndices indices;
	};
}
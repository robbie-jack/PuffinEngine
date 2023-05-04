#pragma once

#include "bgfx/bgfx.h"
#include "Types/Vector.h"
#include "Types/Vertex.hpp"

#include <glm/glm.hpp>
#include <cstdint>

namespace puffin::Rendering
{
	inline bgfx::VertexLayout VertexPC32::GetLayout()
	{
		bgfx::VertexLayout layout;

		layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		return layout;
	}

	inline bgfx::VertexLayout VertexPNTV32::GetLayout()
	{
		bgfx::VertexLayout layout;

		layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		return layout;
	}

	inline bgfx::VertexLayout VertexP64NTV32::GetLayout()
	{
		bgfx::VertexLayout layout;

		layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		return layout;
	}

	static bgfx::VertexLayout s_layoutVertexPC32 = VertexPC32::GetLayout();
	static bgfx::VertexLayout s_layoutVertexPNTV32 = VertexPNTV32::GetLayout();

	typedef std::vector<VertexPC32> VertexPC32Buffer;
	typedef std::vector<VertexPC32> VertexPNTV32Buffer;
	typedef std::vector<VertexP64NTV32> VertexP64NTV32Buffer;

	typedef std::vector<uint32_t> VertexBufferIndices;
}
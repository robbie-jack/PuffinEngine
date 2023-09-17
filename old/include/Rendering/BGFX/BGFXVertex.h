#pragma once

#include "bgfx/bgfx.h"
#include "Types/Vertex.h"

#include <cstdint>

namespace puffin::rendering
{
	inline bgfx::VertexLayout VertexPC32::getLayout()
	{
		bgfx::VertexLayout layout;

		layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		return layout;
	}

	inline bgfx::VertexLayout VertexPNC32::getLayout()
	{
		bgfx::VertexLayout layout;

		layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		return layout;
	}

	inline bgfx::VertexLayout VertexPNTV32::getLayout()
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

	inline bgfx::VertexLayout VertexP64NTV32::getLayout()
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

	const static bgfx::VertexLayout gLayoutVertexPC32 = VertexPC32::getLayout();
	const static bgfx::VertexLayout gLayoutVertexPNTV32 = VertexPNTV32::getLayout();

	using VertexPC32Buffer = std::vector<VertexPC32>;
	using VertexPNTV32Buffer = std::vector<VertexPC32>;
	using VertexP64NTV32Buffer = std::vector<VertexP64NTV32>;

	typedef std::vector<uint32_t> VertexBufferIndices;
}
#pragma once

#include "bgfx/bgfx.h"

#include <cstdint>

namespace Puffin::Rendering::BGFX
{
	enum class VertexFormat : uint8_t
	{
		Unknown = 0,
		PC_32,
		PNCTV_32,
		PNTV_32,
		P_64_NTV_32
	};

	struct VertexPC32
	{
		float x, y, z;
		uint32_t m_abgr;

		static bgfx::VertexLayout GetLayout()
		{
			bgfx::VertexLayout layout;

			layout
				.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
				.end();

			return layout;
		}
	};

	static bgfx::VertexLayout s_layoutVertexPC32 = VertexPC32::GetLayout();
}

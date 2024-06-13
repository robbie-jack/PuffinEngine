#pragma once

#include <cstdint>

namespace puffin::rendering
{
	constexpr uint8_t g_buffered_frames = 2;
	constexpr uint32_t g_max_objects = 20000;
	constexpr uint16_t g_max_materials = 128;
	constexpr uint16_t g_max_unique_materials = 32;
	constexpr uint16_t g_max_lights = 8;
	constexpr uint8_t g_max_shadow_cascades_per_light = 4;
}

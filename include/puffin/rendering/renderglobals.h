#pragma once

#include <cstdint>

namespace puffin::rendering
{
	constexpr uint8_t gBufferedFrames = 2;
	constexpr uint32_t gMaxObjects = 20000;
	constexpr uint16_t gMaxUniqueMaterials = 32;
	constexpr uint16_t gMaxMaterialInstances = 128;
	constexpr uint16_t gMaxPointLights = 8;
	constexpr uint16_t gMaxSpotLights = 8;
	constexpr uint16_t gMaxDirectionalLights = 8;
	constexpr uint16_t gMaxLights = gMaxPointLights + gMaxSpotLights + gMaxDirectionalLights;
	constexpr uint8_t gMaxShadowCascadesPerLight = 4;
}

#pragma once

#include "vulkan/vulkan.hpp"

#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

namespace puffin::rendering
{
	/*
	 * Render module implementation for a 3d forward rendering pass
	 */
	class Forward3DRenderModuleVK : public RenderModuleVK
	{
	public:

		explicit Forward3DRenderModuleVK(RenderSubsystemVK* renderSubsystem);
		~Forward3DRenderModuleVK() override = default;

		void Initialize() override;
		void Deinitialize() override;

		void BuildGraph(RenderGraphVK& renderGraph) override;

		void PreRender(double deltaTime) override;
		
		void RecordForward3DCommands(vk::CommandBuffer& cmd);
	};
}
#pragma once

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

		void Update(double deltaTime) override;
		void PrepareGraph(RenderGraphVK& renderGraph) override;

	};
}
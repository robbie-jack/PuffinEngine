#pragma once

namespace puffin::rendering
{
	class RenderSubsystem;

	class Renderer
	{
	public:

		explicit Renderer(RenderSubsystem* renderSubsystem);
		virtual ~Renderer();

		virtual void Initialize() = 0;
		virtual void Deinitialize() = 0;

		/*
		 * Called by render subsystem to setup render passes, whenever needed
		 */ 
		virtual void SetupRenderPasses() = 0;

		/*
		 * Called once a frame to prepare render commands for parsing
		 */
		virtual void PrepareRenderCommands() = 0;

	protected:

		RenderSubsystem* mRenderSubsystem = nullptr;

	};
}

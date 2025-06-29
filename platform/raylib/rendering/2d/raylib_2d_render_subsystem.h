#pragma once

#include <Camera2D.hpp>
#include <Color.hpp>

#include "rendering/render_subsystem.h"

namespace puffin
{
	namespace utility
	{
		class Benchmark;
	}
}

namespace puffin::rendering
{
	class Raylib2DRenderSubsystem final : public RenderSubsystem
	{
	public:

		explicit Raylib2DRenderSubsystem(const std::shared_ptr<core::Engine>& engine);
		~Raylib2DRenderSubsystem() override = default;

		void RegisterTypes() override;

		void Initialize() override;
		void Deinitialize() override;

		double WaitForLastPresentationAndSampleTime() override;

		void Render(double deltaTime) override;

		void WindowResized(Size size) override;
		void ViewportResized(Size size) override;

		void DrawTextToScreen(const std::string& string, int posX, int posY, int fontSize, Vector3f color) override;

	private:

		struct TextDraw
		{
			TextDraw(const std::string& string, int posX, int posY, int fontSize, raylib::Color color)
				: string(string), posX(posX), posY(posY), fontSize(fontSize), color(color)
			{
			}

			std::string string;
			int posX = 0;
			int posY = 0;
			int fontSize = 0;
			raylib::Color color = {};
		};

		void InitSettingsAndSignals();

		void UpdateCamera();

		void DrawSprites();
		void DrawSpriteNodes() const;
		void DrawSpriteComponents() const;
		void DrawText(const TextDraw& textDraw) const;
		void DebugDrawStats(double deltaTime) const;
		void DebugDrawBenchmark(const utility::Benchmark* benchmark, int posX, int& posY) const;

		[[nodiscard]] float ScaleWorldToPixel(const float& val) const;
		[[nodiscard]] raylib::Vector2 ScaleWorldToPixel(const raylib::Vector2& val) const;

		raylib::Camera2D m_camera;
		int32_t m_pixelScale = 0;

		std::vector<TextDraw> m_textDraws;


	};
}
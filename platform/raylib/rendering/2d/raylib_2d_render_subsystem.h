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

namespace puffin
{
	namespace rendering
	{
		class Raylib2DRenderSubsystem final : public RenderSubsystem
		{
		public:

			explicit Raylib2DRenderSubsystem(const std::shared_ptr<core::Engine>& engine);
			~Raylib2DRenderSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			std::string_view GetName() const override;

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

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::Raylib2DRenderSubsystem>()
		{
			return "Raylib2DRenderSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<rendering::Raylib2DRenderSubsystem>()
		{
			return entt::hs(GetTypeString<rendering::Raylib2DRenderSubsystem>().data());
		}

		template<>
		inline void RegisterType<rendering::Raylib2DRenderSubsystem>()
		{
			auto meta = entt::meta<rendering::Raylib2DRenderSubsystem>()
				.base<rendering::RenderSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
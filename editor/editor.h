#pragma once

#include <memory>

namespace argparse
{
	class ArgumentParser;
}

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace editor
	{
		class Editor : public std::enable_shared_from_this<Editor>
		{
		public:

			Editor();
			~Editor();

			void Initialize(const argparse::ArgumentParser& parser);
			bool Update();
			void Deinitialize();

			[[nodiscard]] std::shared_ptr<core::Engine> GetEngine() const;

		private:

			void RegisterEditorSubsystems();
			void AddEditorContext();
			void InitSignals();

			std::shared_ptr<core::Engine> m_engine;
			size_t m_editorPlayPauseConnectionId = 0;
			size_t m_editorRestartConnectionId = 0;

		};
	}
}

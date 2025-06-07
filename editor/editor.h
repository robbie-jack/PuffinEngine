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

			void Setup();
			void Initialize(const argparse::ArgumentParser& parser);
			bool Update();
			void Deinitialize();

			[[nodiscard]] std::shared_ptr<core::Engine> GetEngine() const;

		private:

			void RegisterEditorSubsystems();

			std::shared_ptr<core::Engine> mEngine;

		};
	}
}

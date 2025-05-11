#include "puffin/editor/editor.h"

#include "puffin/core/engine.h"
#include "puffin/editor/ui/editoruisubsystem.h"

namespace puffin::editor
{
	Editor::Editor()
	{
		mEngine = std::make_shared<puffin::core::Engine>();
	}

	Editor::~Editor()
	{
		mEngine = nullptr;
	}

	void Editor::Setup()
	{
		mEngine->SetEditor(shared_from_this());
		mEngine->Setup();

		RegisterEditorSubsystems();
	}

	void Editor::Initialize(const argparse::ArgumentParser& parser)
	{
		mEngine->Initialize(parser);
	}

	bool Editor::Update()
	{
		return mEngine->Update();
	}

	void Editor::Deinitialize()
	{
		mEngine->Deinitialize();
	}

	std::shared_ptr<core::Engine> Editor::GetEngine() const
	{
		return mEngine;
	}

	void Editor::RegisterEditorSubsystems()
	{
		// Editor Subsystems
		mEngine->RegisterSubsystem<ui::EditorUISubsystem>();
	}
}

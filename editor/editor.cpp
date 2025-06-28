#include "editor.h"

#include "core/engine.h"
#include "ui/editor_ui_subsystem.h"
#include "editor_camera_subsystem.h"
#include "input/input_context.h"
#include "input/input_event.h"
#include "input/input_subsystem.h"
#include "input/input_types.h"

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

		AddEditorContext();
		InitSignals();
	}

	bool Editor::Update()
	{
		return mEngine->Update();
	}

	void Editor::Deinitialize()
	{
		auto inputSubsystem = mEngine->GetInputSubsystem();
		auto editorContext = inputSubsystem->GetContext("editor");

		auto editorPlayPauseSignal = editorContext->GetActionSignal("editor_play_pause");
		editorPlayPauseSignal->Disconnect(mEditorPlayPauseConnectionId);
		mEditorPlayPauseConnectionId = 0;

		auto editorRestartSignal = editorContext->GetActionSignal("editor_restart");
		editorRestartSignal->Disconnect(mEditorRestartConnectionId);
		mEditorRestartConnectionId = 0;

		inputSubsystem->RemoveContext("editor");

		mEngine->Deinitialize();
	}

	std::shared_ptr<core::Engine> Editor::GetEngine() const
	{
		return mEngine;
	}

	void Editor::RegisterEditorSubsystems()
	{
		// Editor Subsystems
		mEngine->RegisterSubsystem<EditorCameraSubsystem>();
		mEngine->RegisterSubsystem<ui::EditorUISubsystem>();
	}

	void Editor::InitSignals()
	{
		auto inputSubsystem = mEngine->GetInputSubsystem();
		auto editorContext = inputSubsystem->GetContext("editor");

		auto editorPlayPauseSignal = editorContext->GetActionSignal("editor_play_pause");
		mEditorPlayPauseConnectionId = editorPlayPauseSignal->Connect(std::function([&](const input::InputEvent& event)
		{
			if (event.actionState == input::InputState::Pressed)
				mEngine->Play();
		}));

		auto editorRestartSignal = editorContext->GetActionSignal("editor_restart");
		mEditorRestartConnectionId = editorRestartSignal->Connect(std::function([&](const input::InputEvent& event)
		{
			if (event.actionState == input::InputState::Pressed)
				mEngine->Restart();
		}));
	}

	void Editor::AddEditorContext()
	{
		auto inputSubsystem = mEngine->GetInputSubsystem();
		auto editorContext = inputSubsystem->AddContext("editor");

		auto& editorCamMoveForwardAction = editorContext->AddAction("editor_cam_move_forward");
		editorCamMoveForwardAction.keys.emplace_back(input::KeyboardKey::W);

		auto& editorCamMoveBackwardAction = editorContext->AddAction("editor_cam_move_backward");
		editorCamMoveBackwardAction.keys.emplace_back(input::KeyboardKey::S);

		auto& editorCamMoveLeftAction = editorContext->AddAction("editor_cam_move_left");
		editorCamMoveLeftAction.keys.emplace_back(input::KeyboardKey::A);

		auto& editorCamMoveRightAction = editorContext->AddAction("editor_cam_move_right");
		editorCamMoveRightAction.keys.emplace_back(input::KeyboardKey::D);

		auto& editorCamMoveUpAction = editorContext->AddAction("editor_cam_move_up");
		editorCamMoveUpAction.keys.emplace_back(input::KeyboardKey::E);

		auto& editorCamMoveDownAction = editorContext->AddAction("editor_cam_move_down");
		editorCamMoveDownAction.keys.emplace_back(input::KeyboardKey::Q);

		auto& editorCamLookAroundAction = editorContext->AddAction("editor_cam_look_around");
		editorCamLookAroundAction.mouseButtons.emplace_back(input::MouseButton::Right);

		auto& editorPlayPause = editorContext->AddAction("editor_play_pause");
		editorPlayPause.keys.emplace_back(input::KeyboardKey::P, true);

		auto& editorRestart = editorContext->AddAction("editor_restart");
		editorRestart.keys.emplace_back(input::KeyboardKey::O, true);
	}
}

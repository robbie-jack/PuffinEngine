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
		m_engine = std::make_shared<puffin::core::Engine>();
	}

	Editor::~Editor()
	{
		m_engine = nullptr;
	}

	void Editor::Initialize(const argparse::ArgumentParser& parser)
	{
		RegisterEditorSubsystems();
		m_engine->SetEditor(shared_from_this());
		m_engine->Initialize(parser);

		AddEditorContext();
		InitSignals();
	}

	bool Editor::Update()
	{
		return m_engine->Update();
	}

	void Editor::Deinitialize()
	{
		auto inputSubsystem = m_engine->GetInputSubsystem();
		auto editorContext = inputSubsystem->GetContext("editor");

		auto editorPlayPauseSignal = editorContext->GetActionSignal("editor_play_pause");
		editorPlayPauseSignal->Disconnect(m_editorPlayPauseConnectionId);
		m_editorPlayPauseConnectionId = 0;

		auto editorRestartSignal = editorContext->GetActionSignal("editor_restart");
		editorRestartSignal->Disconnect(m_editorRestartConnectionId);
		m_editorRestartConnectionId = 0;

		inputSubsystem->RemoveContext("editor");

		m_engine->Deinitialize();
	}

	std::shared_ptr<core::Engine> Editor::GetEngine() const
	{
		return m_engine;
	}

	void Editor::RegisterEditorSubsystems()
	{
		// Editor Subsystems
		reflection::RegisterType<EditorCameraSubsystem>();
		reflection::RegisterType<ui::EditorUISubsystem>();
	}

	void Editor::InitSignals()
	{
		auto inputSubsystem = m_engine->GetInputSubsystem();
		auto editorContext = inputSubsystem->GetContext("editor");

		auto editorPlayPauseSignal = editorContext->GetActionSignal("editor_play_pause");
		m_editorPlayPauseConnectionId = editorPlayPauseSignal->Connect(std::function([&](const input::InputEvent& event)
		{
			if (event.actionState == input::InputState::Pressed)
				m_engine->Play();
		}));

		auto editorRestartSignal = editorContext->GetActionSignal("editor_restart");
		m_editorRestartConnectionId = editorRestartSignal->Connect(std::function([&](const input::InputEvent& event)
		{
			if (event.actionState == input::InputState::Pressed)
				m_engine->Restart();
		}));
	}

	void Editor::AddEditorContext()
	{
		auto inputSubsystem = m_engine->GetInputSubsystem();
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

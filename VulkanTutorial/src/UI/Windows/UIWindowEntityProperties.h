#pragma once

#ifndef UI_WINDOW_ENTITY_PROPERTIES_H
#define UI_WINDOW_ENTITY_PROPERTIES_H

#include "UIWindow.h"

//#include <ECS/ECS.h>
#include <imfilebrowser.h>
#include <Components/Rendering/CameraComponent.h>

#include <memory>

namespace Puffin
{
    namespace UI
    {
        class UIWindowEntityProperties : public UIWindow
        {
		public:

            UIWindowEntityProperties(Core::Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput)
        		: UIWindow(InEngine, InWorld, InInput)
            {
            };

            void Draw(float dt) override;

            inline void SetEntity(ECS::EntityID entity_) { m_entity = entity_; };
            inline void SetFileBrowser(ImGui::FileBrowser* fileDialog_) { fileDialog = fileDialog_; };

            inline bool HasSceneChanged() { return sceneChanged; };

        private:
            ECS::EntityID m_entity = 0;
            ImGui::FileBrowser* fileDialog;

            void DrawTransformUI(ImGuiTreeNodeFlags flags);

            void DrawMeshUI(ImGuiTreeNodeFlags flags);
            void DrawLightUI(ImGuiTreeNodeFlags flags);

            void DrawRigidbody2DUI(ImGuiTreeNodeFlags flags);
            void DrawCircle2DUI(ImGuiTreeNodeFlags flags);
            void DrawBox2DUI(ImGuiTreeNodeFlags flags);

            void DrawScriptUI(ImGuiTreeNodeFlags flags);

            bool modelSelected = false;
            bool textureSelected = false;
            bool positionChanged = false;
            bool sceneChanged = false;
        };
    }
}

#endif // UI_WINDOW_ENTITY_PROPERTIES_H
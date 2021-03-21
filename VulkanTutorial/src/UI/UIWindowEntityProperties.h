#pragma once

#ifndef UI_WINDOW_ENTITY_PROPERTIES_H
#define UI_WINDOW_ENTITY_PROPERTIES_H

#include <UI/UIWindow.h>

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

            UIWindowEntityProperties(Engine* InEngine, std::shared_ptr<ECS::World> InWorld) : UIWindow(InEngine, InWorld)
            {
            };

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

            inline void SetEntity(ECS::Entity entity_) { entity = entity_; };
            inline void SetFileBrowser(ImGui::FileBrowser* fileDialog_) { fileDialog = fileDialog_; };

            inline bool HasSceneChanged() { return sceneChanged; };

        private:
            ECS::Entity entity;
            ImGui::FileBrowser* fileDialog;

            void DrawTransformUI(ImGuiTreeNodeFlags flags);
            void DrawMeshUI(ImGuiTreeNodeFlags flags);
            void DrawLightUI(ImGuiTreeNodeFlags flags);
            void DrawRigidbodyUI(ImGuiTreeNodeFlags flags);
            void DrawScriptUI(ImGuiTreeNodeFlags flags);

            bool modelSelected = false;
            bool textureSelected = false;
            bool positionChanged = false;
            bool sceneChanged = false;
        };
    }
}

#endif // UI_WINDOW_ENTITY_PROPERTIES_H
#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/scene/Scene.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/camera/CameraController.h>
#include <ituGL/utils/DearImGui.h>
#include <ituGL/asset/ModelLoader.h>

#include <map>

class TextureCubemapObject;
class Material;

class MarioDitherDemo : public Application
{
public:
    MarioDitherDemo();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeCamera();
    void InitializeLights();
    void InitializeDefaultMaterial();
    void InitializeFlagDitherMaterial();
    void InitializeMarioDitherMaterial();
    void InitializeMarioPbrMaterial();
    void PrepareLoaderAttributes(ModelLoader* loader);
    void InitializeModels();
    void InitializeRenderer();

    void RenderGUI();

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Camera controller
    CameraController m_cameraController;

    // Global scene
    Scene m_scene;

    // Renderer
    Renderer m_renderer;

    // Skybox texture
    std::shared_ptr<TextureCubemapObject> m_skyboxTexture;

    // Default material
    std::shared_ptr<Material> m_defaultMaterial;
    std::shared_ptr<Material> m_flagDitherMaterial;
    std::shared_ptr<Material> m_marioDitherMaterial;
    std::shared_ptr<Material> m_marioPbrMaterial;

    float m_ditherThreshold = 3.0f;
    float m_ditherScale = 1.0f;
    float m_cameraFlagDistance = 1.0f;
    float m_marioDitherAmount = 0.8f;
};

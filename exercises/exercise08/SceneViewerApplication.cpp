#include "SceneViewerApplication.h"

#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>

#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/lighting/PointLight.h>
#include <ituGL/scene/SceneLight.h>

#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/scene/SceneModel.h>
#include <ituGL/scene/Transform.h>

#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/renderer/ForwardRenderPass.h>
#include "MarioDitherRenderPass.h"
#include <ituGL/scene/RendererSceneVisitor.h>

#include <ituGL/scene/ImGuiSceneVisitor.h>
#include <imgui.h>

#include <map>
#include <string>

SceneViewerApplication::SceneViewerApplication()
    : Application(1024, 1024, "Scene Viewer demo")
    , m_renderer(GetDevice())
{
}

void SceneViewerApplication::Initialize()
{
    Application::Initialize();

    // Initialize DearImGUI
    m_imGui.Initialize(GetMainWindow());

    // Enable Stencil Test
    GetDevice().EnableFeature(GL_STENCIL_TEST);

    InitializeCamera();
    InitializeLights();
    InitializeDefaultMaterial();
    InitializeFlagDitherMaterial();
    InitializeMarioDitherMaterial();
    InitializeMarioPbrMaterial();
    InitializeModels();
    InitializeRenderer();
}

void SceneViewerApplication::Update()
{
    Application::Update();

    // Update camera controller
    m_cameraController.Update(GetMainWindow(), GetDeltaTime());

    // Update camera to flag distance
    glm::vec3 camPos = m_scene.GetSceneNode("camera")->GetTransform()->GetTranslation();
    glm::vec3 flagPos = m_scene.GetSceneNode("Flag")->GetTransform()->GetTranslation();
    m_cameraFlagDistance = glm::distance(camPos, flagPos);

    // Add the scene nodes to the renderer
    RendererSceneVisitor rendererSceneVisitor(m_renderer);
    m_scene.AcceptVisitor(rendererSceneVisitor);
}

void SceneViewerApplication::Render()
{
    Application::Render();

    GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f, true, 0.0f);

    // Render the scene
    m_renderer.Render();

    // Render the debug user interface
    RenderGUI();
}

void SceneViewerApplication::Cleanup()
{
    // Cleanup DearImGUI
    m_imGui.Cleanup();

    Application::Cleanup();
}

void SceneViewerApplication::InitializeCamera()
{
    // Create the main camera
    std::shared_ptr<Camera> camera = std::make_shared<Camera>();
    camera->SetViewMatrix(glm::vec3(-1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    camera->SetPerspectiveProjectionMatrix(1.0f, 1.0f, 0.1f, 1000.0f);

    // Create a scene node for the camera
    std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("camera", camera);

    // Add the camera node to the scene
    m_scene.AddSceneNode(sceneCamera);

    // Set the camera scene node to be controlled by the camera controller
    m_cameraController.SetCamera(sceneCamera);
}

void SceneViewerApplication::InitializeLights()
{
    // Create a directional light and add it to the scene
    std::shared_ptr<DirectionalLight> directionalLight = std::make_shared<DirectionalLight>();
    directionalLight->SetDirection(glm::vec3(-0.3f, -1.0f, -0.3f)); // It will be normalized inside the function
    directionalLight->SetIntensity(3.0f);
    m_scene.AddSceneNode(std::make_shared<SceneLight>("directional light", directionalLight));
}

void SceneViewerApplication::InitializeDefaultMaterial()
{
    // Load and build shader
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/default.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
    fragmentShaderPaths.push_back("shaders/lighting.glsl");
    fragmentShaderPaths.push_back("shaders/default_pbr.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Get transform related uniform locations
    ShaderProgram::Location cameraPositionLocation = shaderProgramPtr->GetUniformLocation("CameraPosition");
    ShaderProgram::Location worldMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldMatrix");
    ShaderProgram::Location viewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("ViewProjMatrix");

    // Register shader with renderer
    m_renderer.RegisterShaderProgram(shaderProgramPtr,
        [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            if (cameraChanged)
            {
                shaderProgram.SetUniform(cameraPositionLocation, camera.ExtractTranslation());
                shaderProgram.SetUniform(viewProjMatrixLocation, camera.GetViewProjectionMatrix());
            }
            shaderProgram.SetUniform(worldMatrixLocation, worldMatrix);
        },
        m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
    );

    // Filter out uniforms that are not material properties
    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("CameraPosition");
    filteredUniforms.insert("WorldMatrix");
    filteredUniforms.insert("ViewProjMatrix");
    filteredUniforms.insert("LightIndirect");
    filteredUniforms.insert("LightColor");
    filteredUniforms.insert("LightPosition");
    filteredUniforms.insert("LightDirection");
    filteredUniforms.insert("LightAttenuation");

    // Create reference material
    assert(shaderProgramPtr);
    m_defaultMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    m_defaultMaterial->SetStencilTestFunction(Material::TestFunction::Always, 1, 0xFF);
}

void SceneViewerApplication::InitializeFlagDitherMaterial() {
    // Load and build shader
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/default.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/map.glsl");
    fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
    fragmentShaderPaths.push_back("shaders/lighting.glsl");
    fragmentShaderPaths.push_back("shaders/bayer_matrix.glsl");
    fragmentShaderPaths.push_back("shaders/dithered_pbr.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Get transform related uniform locations
    ShaderProgram::Location cameraPositionLocation = shaderProgramPtr->GetUniformLocation("CameraPosition");
    ShaderProgram::Location worldMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldMatrix");
    ShaderProgram::Location viewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("ViewProjMatrix");

    // Get dither related uniform locations
    ShaderProgram::Location ditherThresholdLocation = shaderProgramPtr->GetUniformLocation("DitherThreshold");
    ShaderProgram::Location ditherScaleLocation = shaderProgramPtr->GetUniformLocation("DitherScale");
    ShaderProgram::Location camDistanceLocation = shaderProgramPtr->GetUniformLocation("CameraObjectDistance");

    // Register shader with renderer
    m_renderer.RegisterShaderProgram(shaderProgramPtr,
        [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            if (cameraChanged)
            {
                shaderProgram.SetUniform(cameraPositionLocation, camera.ExtractTranslation());
                shaderProgram.SetUniform(viewProjMatrixLocation, camera.GetViewProjectionMatrix());
            }
            shaderProgram.SetUniform(worldMatrixLocation, worldMatrix);

            shaderProgram.SetUniform(ditherThresholdLocation, m_ditherThreshold);
            shaderProgram.SetUniform(ditherScaleLocation, m_ditherScale);
            shaderProgram.SetUniform(camDistanceLocation, m_cameraFlagDistance);
        },
        m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
    );

    // Filter out uniforms that are not material properties
    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("CameraPosition");
    filteredUniforms.insert("WorldMatrix");
    filteredUniforms.insert("ViewProjMatrix");
    filteredUniforms.insert("LightIndirect");
    filteredUniforms.insert("LightColor");
    filteredUniforms.insert("LightPosition");
    filteredUniforms.insert("LightDirection");
    filteredUniforms.insert("LightAttenuation");
    filteredUniforms.insert("DitherThreshold");
    filteredUniforms.insert("DitherScale");
    filteredUniforms.insert("CameraObjectDistance");

    // Create reference material
    assert(shaderProgramPtr);
    m_flagDitherMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);

    // Set Depth and Stencil functions
    m_flagDitherMaterial->SetStencilTestFunction(Material::TestFunction::Always, 1, 0xFF);
    m_flagDitherMaterial->SetStencilOperations(Material::StencilOperation::Keep, Material::StencilOperation::Keep, Material::StencilOperation::Replace);
}

void SceneViewerApplication::InitializeMarioDitherMaterial()
{
    // Load and build shader
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/default.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/map.glsl");
    fragmentShaderPaths.push_back("shaders/bayer_matrix.glsl");
    fragmentShaderPaths.push_back("shaders/mario_dithered.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Get transform related uniform locations
    ShaderProgram::Location cameraPositionLocation = shaderProgramPtr->GetUniformLocation("CameraPosition");
    ShaderProgram::Location worldMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldMatrix");
    ShaderProgram::Location viewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("ViewProjMatrix");

    // Get dither related uniform locations
    ShaderProgram::Location ditherThresholdLocation = shaderProgramPtr->GetUniformLocation("DitherThreshold");
    ShaderProgram::Location ditherScaleLocation = shaderProgramPtr->GetUniformLocation("DitherScale");
    ShaderProgram::Location camDistanceLocation = shaderProgramPtr->GetUniformLocation("CameraObjectDistance");
    ShaderProgram::Location marioDitherLocation = shaderProgramPtr->GetUniformLocation("MarioDitherAmount");

    // Register shader with renderer
    m_renderer.RegisterShaderProgram(shaderProgramPtr,
        [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            if (cameraChanged)
            {
                shaderProgram.SetUniform(cameraPositionLocation, camera.ExtractTranslation());
                shaderProgram.SetUniform(viewProjMatrixLocation, camera.GetViewProjectionMatrix());
            }
            shaderProgram.SetUniform(worldMatrixLocation, worldMatrix);

            shaderProgram.SetUniform(ditherThresholdLocation, m_ditherThreshold);
            shaderProgram.SetUniform(ditherScaleLocation, m_ditherScale);
            shaderProgram.SetUniform(camDistanceLocation, m_cameraFlagDistance);
            shaderProgram.SetUniform(marioDitherLocation, m_marioDitherAmount);
        },
        m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
    );

    // Filter out uniforms that are not material properties
    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("CameraPosition");
    filteredUniforms.insert("WorldMatrix");
    filteredUniforms.insert("ViewProjMatrix");
    filteredUniforms.insert("DitherThreshold");
    filteredUniforms.insert("DitherScale");
    filteredUniforms.insert("CameraObjectDistance");

    // Create reference material
    assert(shaderProgramPtr);
    m_marioDitherMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);

    // Set Depth and Stencil Functions
    m_marioDitherMaterial->SetDepthTestFunction(Material::TestFunction::NotEqual);
    m_marioDitherMaterial->SetStencilTestFunction(Material::TestFunction::Equal, 1, 0xFF);
    m_marioDitherMaterial->SetStencilOperations(Material::StencilOperation::Keep, Material::StencilOperation::Keep, Material::StencilOperation::Keep);
}

void SceneViewerApplication::InitializeMarioPbrMaterial()
{
    m_marioPbrMaterial = std::make_shared<Material>(*m_defaultMaterial);
    m_marioPbrMaterial->SetStencilOperations(Material::StencilOperation::Keep, Material::StencilOperation::Keep, Material::StencilOperation::Zero);
}

void SceneViewerApplication::PrepareLoaderAttributes(ModelLoader* loader)
{
    // Create a new material copy for each submaterial
    loader->SetCreateMaterials(true);

    // Flip vertically textures loaded by the model loader
    loader->GetTexture2DLoader().SetFlipVertical(true);
          
    // Link vertex properties to attributes
    loader->SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
    loader->SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
    loader->SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
    loader->SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
    loader->SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");
          
    // Link material properties to uniforms
    loader->SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
    loader->SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
    loader->SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
    loader->SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");
}

void SceneViewerApplication::InitializeModels()
{
    m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/defaultCubemap.png", TextureObject::FormatRGB, TextureObject::InternalFormatSRGB8);

    m_skyboxTexture->Bind();
    float maxLod;
    m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, maxLod);
    TextureCubemapObject::Unbind();

    m_defaultMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    m_defaultMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

    m_flagDitherMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    m_flagDitherMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

    m_marioPbrMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    m_marioPbrMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

    // Configure Environment loader
    ModelLoader environmentLoader(m_defaultMaterial);
    PrepareLoaderAttributes(&environmentLoader);
    // Configure Flag loader
    ModelLoader flagLoader(m_flagDitherMaterial);
    PrepareLoaderAttributes(&flagLoader);
    // Configure Mario loader
    ModelLoader marioLoader(m_marioPbrMaterial);
    PrepareLoaderAttributes(&marioLoader);

    // Load Environment model
    std::shared_ptr<Model> environmentModel = environmentLoader.LoadShared("models/environment/environment.obj");
    m_scene.AddSceneNode(std::make_shared<SceneModel>("Environment", environmentModel, std::vector<int>{0}));
    std::shared_ptr<Transform> environmentTransform = m_scene.GetSceneNode("Environment")->GetTransform();
    environmentTransform->SetTranslation(glm::vec3(.0f, -19.0f, .0f));
    environmentTransform->SetRotation(glm::vec3(.0f, -1.9f, .0f));

    // Load Flag model
    std::shared_ptr<Model> flagModel = flagLoader.LoadShared("models/flag/flag.obj");
    m_scene.AddSceneNode(std::make_shared<SceneModel>("Flag", flagModel, std::vector<int>{0}));
    std::shared_ptr<Transform> flagTransform = m_scene.GetSceneNode("Flag")->GetTransform();
    flagTransform->SetScale(glm::vec3(.01f));

    // Load Mario model
    std::shared_ptr<Model> marioModel = marioLoader.LoadShared("models/mario/mario.obj");
    m_scene.AddSceneNode(std::make_shared<SceneModel>("Mario", marioModel, std::vector<int>{0, 1}));
    std::shared_ptr<Transform> marioTransform = m_scene.GetSceneNode("Mario")->GetTransform();
    marioTransform->SetTranslation(glm::vec3(.0f, .0f, -2.0f));
    marioTransform->SetScale(glm::vec3(.01f));
}

void SceneViewerApplication::InitializeRenderer()
{
    m_renderer.AddRenderPass(std::make_unique<ForwardRenderPass>());
    m_renderer.AddRenderPass(std::make_unique<MarioDitherRenderPass>(1, *m_marioDitherMaterial));
    m_renderer.AddRenderPass(std::make_unique<SkyboxRenderPass>(m_skyboxTexture));
}

void SceneViewerApplication::RenderGUI()
{
    m_imGui.BeginFrame();

    // Draw GUI for scene nodes, using the visitor pattern
    ImGuiSceneVisitor imGuiVisitor(m_imGui, "Scene");
    m_scene.AcceptVisitor(imGuiVisitor);

    // Draw GUI for camera controller
    m_cameraController.DrawGUI(m_imGui);

    // Draw GUI for dither settings
    if (auto window = m_imGui.UseWindow("Dither Settings"))
    {
        ImGui::SliderFloat("Dither Threshold", &m_ditherThreshold, 0.0f, 10.0f);
        ImGui::SliderFloat("Dither Scale", &m_ditherScale, 0.0f, 1.0f);
        ImGui::SliderFloat("Mario Dither Amount", &m_marioDitherAmount, 0.0f, 1.0f);
    }

    m_imGui.EndFrame();
}
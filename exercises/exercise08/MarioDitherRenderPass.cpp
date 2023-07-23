#include "MarioDitherRenderPass.h"

#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/renderer/Renderer.h>

#include <ituGL/shader/Shader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/shader/ShaderProgram.h>

MarioDitherRenderPass::MarioDitherRenderPass()
    : MarioDitherRenderPass(0)
{
}

MarioDitherRenderPass::MarioDitherRenderPass(int drawcallCollectionIndex)
    : m_drawcallCollectionIndex(drawcallCollectionIndex)
    , m_ditherThresholdLocation(-1)
    , m_ditherScaleLocation(-1)
    , m_camDistanceLocation(-1)
    , m_marioDitherLocation(-1)
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
    fragmentShaderPaths.push_back("shaders/bayer_matrix.glsl");
    fragmentShaderPaths.push_back("shaders/mario_pbr.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);
    
    m_shaderProgram.Build(vertexShader, fragmentShader);

    // Get dither related uniform locations
    m_ditherThresholdLocation = m_shaderProgram.GetUniformLocation("DitherThreshold");
    m_ditherScaleLocation = m_shaderProgram.GetUniformLocation("DitherScale");
    m_camDistanceLocation = m_shaderProgram.GetUniformLocation("CameraObjectDistance");
    m_marioDitherLocation = m_shaderProgram.GetUniformLocation("MarioDitherAmount");
}

void MarioDitherRenderPass::Render()
{
    Renderer& renderer = GetRenderer();

    m_shaderProgram.Use();

    m_uniformMap = m_marioDitherApplication.GetDitherUniforms();
    m_shaderProgram.SetUniform(m_ditherThresholdLocation, m_uniformMap.find("ditherThreshold"));
    m_shaderProgram.SetUniform(m_ditherScaleLocation, m_uniformMap.find("ditherThreshold"));
    m_shaderProgram.SetUniform(m_marioDitherLocation, m_uniformMap.find("marioDitherAmount"));
    m_shaderProgram.SetUniform(m_camDistanceLocation, m_uniformMap.find("cameraFlagDistance"));

    const auto& lights = renderer.GetLights();
    const auto& drawcallCollection = renderer.GetDrawcalls(m_drawcallCollectionIndex);

    // for all drawcalls
    for (const Renderer::DrawcallInfo& drawcallInfo : drawcallCollection)
    {
        if (drawcallInfo.material.GetShaderProgram())
        // Prepare drawcall states
        renderer.PrepareDrawcall(drawcallInfo);

        std::shared_ptr<const ShaderProgram> shaderProgram = drawcallInfo.material.GetShaderProgram();

        //for all lights
        bool first = true;
        unsigned int lightIndex = 0;
        while (renderer.UpdateLights(shaderProgram, lights, lightIndex))
        {
            // Set the renderstates
            renderer.SetLightingRenderStates(first);

            // Draw
            drawcallInfo.drawcall.Draw();

            first = false;
        }
    }
}
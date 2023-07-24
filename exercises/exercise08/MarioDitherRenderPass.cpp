#include "MarioDitherRenderPass.h"

#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/renderer/Renderer.h>

#include <ituGL/shader/Shader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/shader/ShaderProgram.h>
#include <iostream>

MarioDitherRenderPass::MarioDitherRenderPass(int drawcallCollectionIndex, Material& marioDitherMaterial)
    : m_drawcallCollectionIndex(drawcallCollectionIndex)
    , m_marioDitherMaterial(marioDitherMaterial)
{
}

void MarioDitherRenderPass::Render()
{
    Renderer& renderer = GetRenderer();

    const auto& drawcallCollection = renderer.GetDrawcalls(m_drawcallCollectionIndex);

    // for all drawcalls
    for (const Renderer::DrawcallInfo& drawcallInfo : drawcallCollection)
    {
        m_marioDitherMaterial.Use();

        // Prepare drawcall states
        renderer.UpdateTransforms(m_marioDitherMaterial.GetShaderProgram(), drawcallInfo.worldMatrixIndex);
        drawcallInfo.vao.Bind();

        // Draw
        drawcallInfo.drawcall.Draw();
    }
}
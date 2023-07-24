#pragma once

#include "ituGL/renderer/RenderPass.h"
#include <ituGL/shader/ShaderProgram.h>
#include <ituGL/shader/Material.h>

class MarioDitherRenderPass : public RenderPass
{
public:
	MarioDitherRenderPass(int drawcallCollectionIndex, Material& marioDitherMaterial, Material& marioPbrMaterial);

	void Render() override;

private:
	int m_drawcallCollectionIndex;

	Material m_marioDitherMaterial;
	Material m_marioPbrMaterial;
};


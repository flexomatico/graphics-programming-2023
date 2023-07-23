#pragma once

#include "ituGL/renderer/RenderPass.h"
#include <ituGL/shader/ShaderProgram.h>
#include "SceneViewerApplication.h"

#include <map>

class MarioDitherRenderPass : public RenderPass
{
public:
	MarioDitherRenderPass();
	MarioDitherRenderPass(int drawcallCollectionIndex);

	void Render() override;

private:
	int m_drawcallCollectionIndex;

	ShaderProgram m_shaderProgram;
	ShaderProgram::Location m_ditherThresholdLocation;
	ShaderProgram::Location m_ditherScaleLocation;
	ShaderProgram::Location m_camDistanceLocation;
	ShaderProgram::Location m_marioDitherLocation;

	std::map<std::string, float> m_uniformMap;

	SceneViewerApplication m_marioDitherApplication;
};


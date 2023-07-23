//Inputs
in vec3 WorldPosition;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangent;
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform vec3 Color;
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;

uniform vec3 CameraPosition;

uniform float DitherThreshold;
uniform float DitherScale;
uniform float CameraObjectDistance;
uniform float MarioDitherAmount;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main()
{
	float clampedDistance = clamp(CameraObjectDistance, 0, DitherThreshold);
	float mappedDistance01 = map(clampedDistance, 0, DitherThreshold, 0, 1);

	if (mappedDistance01 < 1)
		discard;

	bool keep = dither8x8(gl_FragCoord.xy, MarioDitherAmount, DitherScale);
	if (keep)
		discard;

//	SurfaceData data;
//	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
//	data.albedo = Color * texture(ColorTexture, TexCoord).rgb;
//	vec3 arm = texture(SpecularTexture, TexCoord).rgb;
//	data.ambientOcclusion = arm.x;
//	data.roughness = arm.y;
//	data.metalness = arm.z;
//
//	vec3 position = WorldPosition;
//	vec3 viewDir = GetDirection(position, CameraPosition);
//	vec3 color = ComputeLighting(position, data, viewDir, true);
	FragColor = vec4(0, 0, 0, 1);
}
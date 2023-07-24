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

void main()
{
	float clampedDistance = clamp(CameraObjectDistance, 0, DitherThreshold);
	float mappedDistance01 = map(clampedDistance, 0, DitherThreshold, 0, 1); 
	bool keep = dither8x8(gl_FragCoord.xy, mappedDistance01, DitherScale);
	if (!keep)
		discard;

	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	data.albedo = Color * texture(ColorTexture, TexCoord).rgb;
	vec3 arm = texture(SpecularTexture, TexCoord).rgb;
	data.ambientOcclusion = arm.x;
	data.roughness = arm.y;
	data.metalness = arm.z;

	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);
	vec3 color = ComputeLighting(position, data, viewDir, true);
	FragColor = vec4(color.rgb, 1);
}
//Outputs
out vec4 FragColor;

uniform float DitherThreshold;
uniform float DitherScale;
uniform float CameraObjectDistance;
uniform float MarioDitherAmount;

void main()
{
	float clampedDistance = clamp(CameraObjectDistance, 0, DitherThreshold);
	float mappedDistance01 = map(clampedDistance, 0, DitherThreshold, 0, 1);

	if (mappedDistance01 < 1)
		discard;

	bool keep = dither8x8(gl_FragCoord.xy, MarioDitherAmount, DitherScale);
	if (keep)
		discard;

	FragColor = vec4(0, 0, 0, 1);
}
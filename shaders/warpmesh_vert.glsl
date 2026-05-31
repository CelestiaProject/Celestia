layout(location = 0) in vec2 in_Position;
layout(location = 2) in vec2 in_TexCoord0;
layout(location = 9) in float in_Intensity;

out vec2 texCoord;
out float intensity;

uniform float screenRatio;

void main(void)
{
    float offset = 0.5 - screenRatio * 0.5;
    gl_Position = vec4(in_Position.x * screenRatio, in_Position.y, 0.0, 1.0);
    texCoord = vec2(in_TexCoord0.x * screenRatio + offset, in_TexCoord0.y);
    intensity = in_Intensity;
}

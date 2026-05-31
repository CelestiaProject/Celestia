layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec3 in_Normal;
in float in_Brightness;

uniform vec3 color;
uniform vec3 viewDir;
uniform float fadeFactor;

out float shade;

void main(void)
{
    shade = abs(dot(viewDir.xyz, in_Normal.xyz) * in_Brightness * fadeFactor);
    set_vp(in_Position);
}

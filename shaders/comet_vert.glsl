#version 120

attribute float brightness;

uniform vec3 color;
uniform vec3 viewDir;
uniform float fadeFactor;

varying float shade;

void main(void)
{
    shade = abs(dot(viewDir.xyz, gl_Normal.xyz) * brightness * fadeFactor);
    gl_Position = ftransform();
}

attribute vec4 in_Position;
attribute vec3 in_Normal;
attribute float brightness;

uniform vec3 color;
uniform vec3 viewDir;
uniform float fadeFactor;

varying float shade;

void main(void)
{
    shade = abs(dot(viewDir.xyz, in_Normal.xyz) * brightness * fadeFactor);
    gl_Position = MVPMatrix * in_Position;
}

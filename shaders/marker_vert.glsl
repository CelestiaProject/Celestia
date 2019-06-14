#version 120
uniform float s;

void main(void)
{
    vec4 p = vec4(gl_Vertex.xy * s, 0.0f, 1.0f);
    gl_Position = gl_ModelViewProjectionMatrix * p;
}

#version 120

attribute vec2 in_Position;

void main(void)
{
    vec4 p = vec4(in_Position.x, 0.0f, in_Position.y, 1.0f);
    gl_Position = gl_ModelViewProjectionMatrix * p;
}

#version 120

uniform float radius;
uniform float width;
uniform float h;
uniform float angle;

void main(void)
{
    float c = cos(angle);
    float s = sin(angle);
    mat3 rot = mat3(c, s, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 1.0f);

    float x = gl_Vertex.x * width + radius;
    float y = gl_Vertex.y * h;
    vec3 p = rot * vec3(x, y, 0.0f);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0f);
}

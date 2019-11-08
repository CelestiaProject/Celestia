#version 120

void main(void)
{
    vec4 p = vec4(gl_Vertex.x, 0.0f, gl_Vertex.y, 1.0f);
    gl_Position = gl_ModelViewProjectionMatrix * p;
}

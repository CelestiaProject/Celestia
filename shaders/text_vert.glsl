layout(location = 0) in vec2 in_VertexPos;
layout(location = 1) in vec2 in_InstancePos;
layout(location = 2) in vec2 in_Size;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec2 in_TexSize;
layout(location = 8) in vec4 in_Color;

out vec2 texCoord;
out vec4 color;

void main(void)
{
    gl_Position = MVPMatrix * vec4(in_InstancePos + in_Size * in_VertexPos, 0.0, 1.0);
    texCoord = in_TexCoord + in_TexSize * vec2(in_VertexPos.x, 1.0 - in_VertexPos.y);
    color = in_Color;
}

layout(location = 0) in vec2 in_Position;
layout(location = 2) in vec2 in_TexCoord0;
layout(location = 8) in vec4 in_Color;

out vec2 texCoord;
out vec4 color;

void main(void)
{
    gl_Position = MVPMatrix * vec4(in_Position.xy, 0, 1);
    texCoord = in_TexCoord0.st;
    color = in_Color;
}

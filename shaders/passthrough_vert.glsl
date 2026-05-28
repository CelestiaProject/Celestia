layout(location = 0) in vec2 in_Position;
layout(location = 2) in vec2 in_TexCoord0;

out vec2 texCoord;

void main(void)
{
    gl_Position = vec4(in_Position.xy, 0.0, 1.0);
    texCoord = in_TexCoord0.st;
}

attribute vec2 in_Position;
attribute vec2 in_TexCoord0;

varying vec2 texCoord;

void main(void)
{
    gl_Position = vec4(in_Position.xy, 0.0, 1.0);
    texCoord = in_TexCoord0.st;
}

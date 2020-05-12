attribute vec2 in_Position;
attribute vec2 in_TexCoord0;
attribute vec4 in_Color;

varying vec2 texCoord;
varying vec4 color;

void main(void)
{
    gl_Position = MVPMatrix * vec4(in_Position.xy, 0, 1);
    texCoord = in_TexCoord0.st;
    color = in_Color;
}

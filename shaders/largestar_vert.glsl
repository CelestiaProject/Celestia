attribute vec3 in_Position;
attribute vec2 in_TexCoord0;

varying vec2 texCoord;

void main(void)
{
    texCoord = in_TexCoord0.st;
    gl_Position = MVPMatrix * vec4(in_Position, 1.0);
}

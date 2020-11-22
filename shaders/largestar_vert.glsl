attribute vec3 in_Position;
attribute vec2 in_TexCoord0;

uniform float pointWidth;
uniform float pointHeight;
uniform vec3 center;

varying vec2 texCoord;

void main(void)
{
    texCoord = in_TexCoord0.st;
    set_vp(vec4(center, 1.0));
    vec2 transformed = vec2((in_TexCoord0.x - 0.5) * pointWidth, (in_TexCoord0.y - 0.5) * pointHeight);
    gl_Position.xy += transformed * gl_Position.w;
}

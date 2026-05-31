in vec2 texCoord;
in vec4 color;

uniform sampler2D atlasTex;

void main(void)
{
    fragColor = vec4(color.rgb, texture(atlasTex, texCoord).r * color.a);
}

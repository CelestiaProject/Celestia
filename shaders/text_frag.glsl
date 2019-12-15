#version 120

varying vec2 texCoord;
varying vec4 color;

uniform sampler2D atlasTex;

void main(void)
{
    gl_FragColor = vec4(color.rgb, texture2D(atlasTex, texCoord).a * color.a);
}

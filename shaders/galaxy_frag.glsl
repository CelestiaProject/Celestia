#version 120

varying vec4 color;
varying vec2 texCoord;

uniform sampler2D galaxyTex;

void main(void)
{
    gl_FragColor = texture2D(galaxyTex, texCoord) * color;
}

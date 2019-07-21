#version 110

uniform sampler2D tex;
uniform vec4 color;
varying vec2 texCoord;

void main(void)
{
    gl_FragColor = texture2D(tex, texCoord) * color;
}

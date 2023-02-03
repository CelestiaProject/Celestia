varying vec4 color;
varying vec2 texCoord;

uniform sampler2D galaxyTex;

void main(void)
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(galaxyTex, texCoord).r);
}

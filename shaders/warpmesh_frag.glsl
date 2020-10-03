varying vec2 texCoord;
varying float intensity;

uniform sampler2D tex;

void main(void)
{
    gl_FragColor = vec4(texture2D(tex, texCoord).rgb * intensity, 1.0);
}

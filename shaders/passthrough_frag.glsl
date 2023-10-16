varying vec2 texCoord;

uniform sampler2D tex;

const float gamma = 2.2;

float to_srgb(float v)
{
    return v <= 0.0031308 ? 12.92 * v : 1.055 * pow(v, 1.0/2.4) - 0.055;
}

void main(void)
{
//    gl_FragColor = texture2D(tex, texCoord);
    vec3 color = texture2D(tex, texCoord).rgb;
//    gl_FragColor = vec4(pow(color, vec3(1.0 / gamma)), 1.0);
    gl_FragColor = vec4(to_srgb(color.r), to_srgb(color.g), to_srgb(color.b), 1.0);
}

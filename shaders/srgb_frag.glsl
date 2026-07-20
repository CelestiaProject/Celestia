in vec2 texCoord;

uniform sampler2D tex;

// Exposure for tone mapping.
uniform float exposure;

// 0 skips tone mapping (linear light hard-clips on output); non-zero applies it.
uniform float toneMapping;

// Apply the sRGB electro-optical transfer function (IEC 61966-2-1).
// Input is assumed to be linear light; output is gamma-encoded for display.
vec3 linearToSRGB(vec3 c)
{
    // Clamp negatives to avoid undefined pow() behaviour
    c = max(c, vec3(0.0));
    vec3 higher = vec3(1.055) * pow(c, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower  = vec3(12.92) * c;
    // step(edge, x): 0 when x < edge, 1 when x >= edge
    return mix(lower, higher, step(vec3(0.0031308), c));
}

void main(void)
{
    vec4 color = texture(tex, texCoord);
    // Exponential tone mapping to roll off HDR highlights, unless disabled.
    vec3 mapped = color.rgb;
    if (toneMapping != 0.0)
        mapped = vec3(1.0) - exp(-exposure * color.rgb);
    fragColor = vec4(linearToSRGB(mapped), color.a);
}
